#!/usr/bin/env python3
"""Render the qiptv explainer video (motion graphics) with pycairo + ffmpeg.

The UI in the video is an illustration of qiptv's real layout and behaviour,
drawn as vector graphics; it is not a screen recording.

Usage:
    python3 tools/explainer/render.py OUT.mp4 [--poster OUT.jpg] [--preview SECONDS]

Requires: pycairo (python3-cairo), ffmpeg with libx264, IBM Plex Sans/Mono
fonts (falls back to the fontconfig default if missing).
"""

import argparse
import math
import shutil
import subprocess
import sys

import cairo

W, H, FPS = 1280, 720, 30

# Palette: the icon's Breeze blue on a near-black slate.
BG = (0.059, 0.071, 0.086)
PANEL = (0.106, 0.122, 0.141)
PANEL_2 = (0.137, 0.157, 0.180)
LINE = (0.204, 0.227, 0.255)
TEXT = (0.925, 0.937, 0.949)
MUTED = (0.580, 0.612, 0.651)
BLUE = (0.239, 0.682, 0.914)
BLUE_DEEP = (0.114, 0.600, 0.953)
AMBER = (0.965, 0.722, 0.271)

SANS = "IBM Plex Sans"
MONO = "IBM Plex Mono"


# ---------------------------------------------------------------- helpers ---

def clamp(x, lo=0.0, hi=1.0):
    return max(lo, min(hi, x))


def ease(x):
    """Cubic ease-in-out on [0,1]."""
    x = clamp(x)
    return 4 * x * x * x if x < 0.5 else 1 - (-2 * x + 2) ** 3 / 2


def ease_out(x):
    x = clamp(x)
    return 1 - (1 - x) ** 3


def prog(t, start, dur):
    """Eased progress of an animation that starts at `start` and lasts `dur`."""
    return ease((t - start) / dur) if dur > 0 else float(t >= start)


def rgba(c, a=1.0):
    return (*c, a)


def rrect(cr, x, y, w, h, r):
    r = min(r, w / 2, h / 2)
    cr.new_sub_path()
    cr.arc(x + w - r, y + r, r, -math.pi / 2, 0)
    cr.arc(x + w - r, y + h - r, r, 0, math.pi / 2)
    cr.arc(x + r, y + h - r, r, math.pi / 2, math.pi)
    cr.arc(x + r, y + r, r, math.pi, 3 * math.pi / 2)
    cr.close_path()


def fill_rrect(cr, x, y, w, h, r, color, a=1.0):
    rrect(cr, x, y, w, h, r)
    cr.set_source_rgba(*rgba(color, a))
    cr.fill()


def stroke_rrect(cr, x, y, w, h, r, color, a=1.0, lw=1.0):
    rrect(cr, x, y, w, h, r)
    cr.set_source_rgba(*rgba(color, a))
    cr.set_line_width(lw)
    cr.stroke()


def font(cr, size, bold=False, mono=False):
    cr.select_font_face(MONO if mono else SANS, cairo.FONT_SLANT_NORMAL,
                        cairo.FONT_WEIGHT_BOLD if bold else cairo.FONT_WEIGHT_NORMAL)
    cr.set_font_size(size)


def text(cr, s, x, y, size, color=TEXT, a=1.0, bold=False, mono=False, align="left"):
    font(cr, size, bold, mono)
    ext = cr.text_extents(s)
    if align == "center":
        x -= ext.x_advance / 2
    elif align == "right":
        x -= ext.x_advance
    cr.move_to(x, y)
    cr.set_source_rgba(*rgba(color, a))
    cr.show_text(s)
    return ext.x_advance


def text_width(cr, s, size, bold=False, mono=False):
    font(cr, size, bold, mono)
    return cr.text_extents(s).x_advance


def wrap(cr, s, width, size, bold=False):
    font(cr, size, bold)
    lines, cur = [], ""
    for word in s.split():
        trial = (cur + " " + word).strip()
        if cr.text_extents(trial).x_advance > width and cur:
            lines.append(cur)
            cur = word
        else:
            cur = trial
    if cur:
        lines.append(cur)
    return lines


def paragraph(cr, s, x, y, width, size, color=MUTED, a=1.0, leading=1.45):
    for i, line in enumerate(wrap(cr, s, width, size)):
        text(cr, line, x, y + i * size * leading, size, color, a)


# ------------------------------------------------------------------ icons ---

def tri(cr, x, y, w, h, right=True):
    if right:
        cr.move_to(x, y)
        cr.line_to(x + w, y + h / 2)
        cr.line_to(x, y + h)
    else:
        cr.move_to(x + w, y)
        cr.line_to(x, y + h / 2)
        cr.line_to(x + w, y + h)
    cr.close_path()


def icon(cr, name, cx, cy, s, color=TEXT, a=1.0):
    """Media-control glyphs centred on (cx, cy), roughly s px tall."""
    cr.save()
    cr.set_source_rgba(*rgba(color, a))
    h = s
    if name == "play":
        tri(cr, cx - h * 0.35, cy - h / 2, h * 0.8, h)
        cr.fill()
    elif name == "pause":
        cr.rectangle(cx - h * 0.4, cy - h / 2, h * 0.28, h)
        cr.rectangle(cx + h * 0.12, cy - h / 2, h * 0.28, h)
        cr.fill()
    elif name == "stop":
        cr.rectangle(cx - h * 0.42, cy - h * 0.42, h * 0.84, h * 0.84)
        cr.fill()
    elif name in ("next", "prev"):
        right = name == "next"
        tri(cr, cx - h * 0.45, cy - h / 2, h * 0.7, h, right)
        cr.fill()
        bx = cx + h * 0.3 if right else cx - h * 0.45
        cr.rectangle(bx, cy - h / 2, h * 0.15, h)
        cr.fill()
    elif name in ("ffwd", "rew"):
        right = name == "ffwd"
        for dx in (-h * 0.5, 0):
            tri(cr, cx + (dx if right else -dx - h * 0.5), cy - h * 0.4, h * 0.5, h * 0.8, right)
            cr.fill()
    elif name == "back":
        cr.set_line_width(h * 0.14)
        cr.set_line_cap(cairo.LINE_CAP_ROUND)
        cr.set_line_join(cairo.LINE_JOIN_ROUND)
        cr.move_to(cx + h * 0.4, cy)
        cr.line_to(cx - h * 0.4, cy)
        cr.move_to(cx - h * 0.05, cy - h * 0.35)
        cr.line_to(cx - h * 0.4, cy)
        cr.line_to(cx - h * 0.05, cy + h * 0.35)
        cr.stroke()
    elif name == "volume":
        cr.move_to(cx - h * 0.5, cy - h * 0.18)
        cr.line_to(cx - h * 0.25, cy - h * 0.18)
        cr.line_to(cx + h * 0.05, cy - h * 0.45)
        cr.line_to(cx + h * 0.05, cy + h * 0.45)
        cr.line_to(cx - h * 0.25, cy + h * 0.18)
        cr.line_to(cx - h * 0.5, cy + h * 0.18)
        cr.close_path()
        cr.fill()
        cr.set_line_width(h * 0.1)
        cr.set_line_cap(cairo.LINE_CAP_ROUND)
        for r in (0.25, 0.45):
            cr.arc(cx + h * 0.05, cy, h * r, -0.8, 0.8)
            cr.stroke()
    elif name == "fullscreen":
        cr.set_line_width(h * 0.12)
        q = h * 0.42
        for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1)):
            cr.move_to(cx + sx * q, cy + sy * q * 0.35)
            cr.line_to(cx + sx * q, cy + sy * q)
            cr.line_to(cx + sx * q * 0.35, cy + sy * q)
        cr.stroke()
    cr.restore()


def logo(cr, cx, cy, size, t=0.0, waves=1.0):
    """The qiptv icon (data/icons/*.svg) redrawn in cairo; 128-unit viewbox."""
    k = size / 128.0
    cr.save()
    cr.translate(cx - 64 * k, cy - 64 * k)
    cr.scale(k, k)
    fill_rrect(cr, 48, 96, 32, 8, 2, (0.302, 0.302, 0.302))
    fill_rrect(cr, 36, 102, 56, 8, 4, (0.361, 0.361, 0.361))
    fill_rrect(cr, 12, 20, 104, 74, 8, (0.192, 0.212, 0.231))
    g = cairo.LinearGradient(0, 28, 0, 86)
    g.add_color_stop_rgb(0, *BLUE)
    g.add_color_stop_rgb(1, *BLUE_DEEP)
    rrect(cr, 20, 28, 88, 58, 4)
    cr.set_source(g)
    cr.fill()
    cr.set_source_rgb(1, 1, 1)
    cr.move_to(54, 44)
    cr.line_to(54, 70)
    cr.line_to(76, 57)
    cr.close_path()
    cr.fill()
    cr.set_line_width(4)
    cr.set_line_cap(cairo.LINE_CAP_ROUND)
    for i, (r, op) in enumerate(((14, 1.0), (22, 0.6))):
        pulse = clamp(waves * 2 - i)
        cr.set_source_rgba(*rgba(BLUE, op * pulse))
        cr.arc(92, 32, r, -math.pi / 2, 0)
        cr.stroke()
    cr.restore()


def keycap(cr, label, x, y, a=1.0, pressed=0.0, w=None, glyph=None):
    """A keyboard key; `glyph` draws a media icon instead of the label text."""
    size = 18
    w = w or max(44, text_width(cr, label, size, bold=True, mono=True) + 26)
    h = 44
    dy = 3 * pressed
    fill_rrect(cr, x, y + 4, w, h, 8, (0.02, 0.03, 0.04), 0.9 * a)
    fill_rrect(cr, x, y + dy, w, h, 8, PANEL_2, a)
    stroke_rrect(cr, x + 0.5, y + dy + 0.5, w - 1, h - 1, 8,
                 BLUE if pressed > 0.05 else LINE, a, 1.5)
    color = BLUE if pressed > 0.05 else TEXT
    if glyph:
        icon(cr, glyph, x + w / 2, y + dy + h / 2, 16, color, a)
    else:
        text(cr, label, x + w / 2, y + dy + 29, size, color, a, bold=True, mono=True,
             align="center")
    return w


def window(cr, x, y, w, h, title, a=1.0):
    # soft shadow
    for i in range(12, 0, -3):
        fill_rrect(cr, x - i, y - i + 8, w + 2 * i, h + 2 * i, 14 + i, (0, 0, 0), 0.05 * a)
    fill_rrect(cr, x, y, w, h, 10, PANEL, a)
    cr.save()
    rrect(cr, x, y, w, h, 10)
    cr.clip()
    cr.rectangle(x, y, w, 34)
    cr.set_source_rgba(*rgba(PANEL_2, a))
    cr.fill()
    cr.restore()
    text(cr, title, x + w / 2, y + 22, 13, MUTED, a, align="center")
    for i in range(3):
        cr.arc(x + w - 20 - i * 20, y + 17, 5, 0, 2 * math.pi)
        cr.set_source_rgba(*rgba(LINE, a))
        cr.fill()
    stroke_rrect(cr, x + 0.5, y + 0.5, w - 1, h - 1, 10, LINE, a)


def heading(cr, kicker, title, body, a, t, x=80, y=150, width=430):
    """Left-column copy block with a staggered entrance."""
    k1, k2, k3 = (prog(t, d, 0.6) for d in (0.0, 0.15, 0.3))
    text(cr, kicker, x, y + 12 * (1 - k1), 15, BLUE, a * k1, bold=True, mono=True)
    lines = wrap(cr, title, width, 40, bold=True)
    for i, line in enumerate(lines):
        text(cr, line, x, y + 58 + i * 50 + 12 * (1 - k2), 40, TEXT, a * k2, bold=True)
    by = y + 58 + len(lines) * 50 + 14
    cr.save()
    cr.translate(0, 12 * (1 - k3))
    paragraph(cr, body, x, by, width, 19, MUTED, a * k3)
    cr.restore()
    return by + len(wrap(cr, body, width, 19)) * 19 * 1.45


def background(cr, t):
    # A gentle vertical gradient. (A faint radial glow looked nicer in stills
    # but banded into visible rings after 8-bit 4:2:0 encoding.)
    g = cairo.LinearGradient(0, 0, 0, H)
    g.add_color_stop_rgb(0, 0.078, 0.094, 0.114)
    g.add_color_stop_rgb(1, *BG)
    cr.set_source(g)
    cr.paint()


# ----------------------------------------------------------------- scenes ---
# Each scene: draw(cr, t, a) with t = seconds into the scene, a = scene alpha.

def scene_title(cr, t, a):
    s = ease_out(t / 1.2)
    logo(cr, W / 2, 270, 150 * (0.85 + 0.15 * s), t, waves=clamp((t - 0.8) / 0.8))
    k = prog(t, 0.6, 0.8)
    text(cr, "qiptv", W / 2, 420 + 14 * (1 - k), 76, TEXT, a * k, bold=True, align="center")
    k = prog(t, 1.1, 0.8)
    text(cr, "A native Linux player for Xtream Codes IPTV", W / 2, 470 + 10 * (1 - k),
         24, MUTED, a * k, align="center")
    k = prog(t, 1.6, 0.8)
    text(cr, "C++  ·  Qt 6  ·  libmpv", W / 2, 520, 16, BLUE, a * k, mono=True, align="center")


def scene_login(cr, t, a):
    heading(cr, "01 — SIGN IN", "Your provider, your account.",
            "Enter the server URL, username and password your provider gave you. "
            "Save several profiles; qiptv signs back in to the last one.", a, t)
    wx, wy, ww, wh = 640, 120, 540, 470
    k = prog(t, 0.2, 0.7)
    cr.save()
    cr.translate(0, 30 * (1 - k))
    window(cr, wx, wy, ww, wh, "qiptv", a * k)
    fields = [("Server URL", "http://your-provider:8080", 0.9),
              ("Username", "alex", 2.4),
              ("Password", "••••••••••", 3.0)]
    fy = wy + 70
    for label, value, start in fields:
        text(cr, label, wx + 40, fy, 14, MUTED, a * k)
        focused = start <= t < start + 1.2
        fill_rrect(cr, wx + 40, fy + 10, ww - 80, 42, 6, BG, a * k)
        stroke_rrect(cr, wx + 40.5, fy + 10.5, ww - 81, 41, 6, BLUE if focused else LINE,
                     a * k, 1.5 if focused else 1)
        n = int(len(value) * clamp((t - start) / (0.9 if len(value) > 12 else 0.5)))
        typed = value[:n]
        tw = text(cr, typed, wx + 54, fy + 37, 17, TEXT, a * k, mono=True)
        if focused and int(t * 2.5) % 2 == 0:
            cr.rectangle(wx + 56 + tw, fy + 21, 2, 22)
            cr.set_source_rgba(*rgba(BLUE, a * k))
            cr.fill()
        fy += 86
    # "Save this profile" checkbox
    cy = fy - 6
    checked = t > 3.8
    fill_rrect(cr, wx + 40, cy, 20, 20, 4, BLUE if checked else BG, a * k)
    stroke_rrect(cr, wx + 40.5, cy + 0.5, 19, 19, 4, BLUE if checked else LINE, a * k)
    if checked:
        cr.set_source_rgba(1, 1, 1, a * k)
        cr.set_line_width(2.5)
        cr.move_to(wx + 45, cy + 10)
        cr.line_to(wx + 49, cy + 14)
        cr.line_to(wx + 56, cy + 6)
        cr.stroke()
    text(cr, "Save this profile", wx + 72, cy + 16, 15, TEXT, a * k)
    # Sign-in button with a press at 4.4s
    press = clamp(1 - abs(t - 4.5) / 0.15)
    bx, by, bw, bh = wx + ww - 180, cy + 44, 140, 44
    fill_rrect(cr, bx, by + 2 * press, bw, bh, 8, BLUE_DEEP if press else BLUE, a * k)
    text(cr, "Sign in", bx + bw / 2, by + 28 + 2 * press, 17, (1, 1, 1), a * k, bold=True,
         align="center")
    cr.restore()
    # Saved-profile chips slide in afterwards
    for i, name in enumerate(("Home", "Travel", "Living room")):
        kk = prog(t, 5.0 + i * 0.2, 0.5)
        x = 80 + sum(text_width(cr, n, 16) + 56 for n in ("Home", "Travel", "Living room")[:i])
        w = text_width(cr, name, 16) + 40
        fill_rrect(cr, x, 470 + 14 * (1 - kk), w, 38, 19, PANEL_2, a * kk)
        stroke_rrect(cr, x + 0.5, 470.5 + 14 * (1 - kk), w - 1, 37, 19, BLUE if i == 0 else LINE,
                     a * kk)
        text(cr, name, x + w / 2, 495 + 14 * (1 - kk), 16, TEXT, a * kk, align="center")
    text(cr, "SAVED PROFILES", 80, 455, 12, MUTED, a * prog(t, 5.0, 0.5), mono=True)


POSTER_COLORS = [(0.86, 0.36, 0.33), (0.26, 0.62, 0.53), (0.93, 0.62, 0.25), (0.45, 0.42, 0.82),
                 (0.25, 0.55, 0.85), (0.80, 0.40, 0.62), (0.40, 0.65, 0.35), (0.62, 0.48, 0.32)]
CHANNELS = ["News 24", "Sport One", "Sport Two", "Kids Club", "Docu World", "Music Box",
            "Sport Extra", "Cinema 1", "Arena Sport", "Nature HD", "Retro TV", "Sport Max"]


def scene_browse(cr, t, a):
    heading(cr, "02 — BROWSE", "Live TV, movies and series by category.",
            "Pick a section, pick a category, and the list fills in with logos and posters. "
            "Start typing to filter what you see.", a, t, width=400)
    wx, wy, ww, wh = 520, 90, 700, 540
    k = prog(t, 0.2, 0.7)
    cr.save()
    cr.translate(0, 30 * (1 - k))
    window(cr, wx, wy, ww, wh, "qiptv — Browse", a * k)
    # Section rail
    sections = ["Live TV", "Movies", "Series"]
    sel = 0
    for i, s in enumerate(sections):
        y = wy + 60 + i * 44
        if i == sel:
            fill_rrect(cr, wx + 12, y - 24, 120, 36, 6, BLUE, 0.18 * a * k)
            cr.rectangle(wx + 12, y - 24, 3, 36)
            cr.set_source_rgba(*rgba(BLUE, a * k))
            cr.fill()
        text(cr, s, wx + 28, y, 15, TEXT if i == sel else MUTED, a * k, bold=i == sel)
    cr.rectangle(wx + 144, wy + 34, 1, wh - 34)
    cr.set_source_rgba(*rgba(LINE, a * k))
    cr.fill()
    # Categories with a selection that moves News -> Sports at 1.6s
    cats = ["All", "News", "Sports", "Kids", "Documentary", "Music"]
    cat_sel = 2 if t > 1.6 else 1
    for i, c in enumerate(cats):
        y = wy + 60 + i * 38
        if i == cat_sel:
            fill_rrect(cr, wx + 154, y - 22, 150, 32, 6, PANEL_2, a * k)
        text(cr, c, wx + 168, y, 14, TEXT if i == cat_sel else MUTED, a * k)
    cr.rectangle(wx + 314, wy + 34, 1, wh - 34)
    cr.set_source_rgba(*rgba(LINE, a * k))
    cr.fill()
    # Search box types "sport" from 3.2s
    query = "sport"[:int(5 * clamp((t - 3.2) / 0.8))]
    sx, sy = wx + 330, wy + 48
    fill_rrect(cr, sx, sy, ww - 346, 36, 6, BG, a * k)
    stroke_rrect(cr, sx + 0.5, sy + 0.5, ww - 347, 35, 6, BLUE if query else LINE, a * k)
    text(cr, query or "Search…", sx + 14, sy + 24, 15, TEXT if query else MUTED, a * k)
    # Content grid
    visible = [c for c in CHANNELS if query.lower() in c.lower()] if len(query) == 5 else CHANNELS
    gx, gy = wx + 330, wy + 100
    tw, th, gap = 110, 96, 12
    for i, name in enumerate(visible):
        col, row = i % 3, i // 3
        appear = prog(t, 0.6 + i * 0.06, 0.4) if len(query) < 5 else prog(t, 4.1 + i * 0.05, 0.3)
        x = gx + col * (tw + gap)
        y = gy + row * (th + gap)
        if y + th > wy + wh - 10:
            break
        color = POSTER_COLORS[CHANNELS.index(name) % len(POSTER_COLORS)]
        g = cairo.LinearGradient(x, y, x + tw, y + th)
        g.add_color_stop_rgba(0, *color, 0.95 * a * k * appear)
        g.add_color_stop_rgba(1, *[c * 0.55 for c in color], 0.95 * a * k * appear)
        rrect(cr, x, y + 8 * (1 - appear), tw, th - 26, 6)
        cr.set_source(g)
        cr.fill()
        text(cr, name, x + 4, y + th - 6 + 8 * (1 - appear), 13, TEXT, a * k * appear)
    cr.restore()


def scene_player(cr, t, a):
    end = heading(cr, "03 — PLAY", "Smooth playback with libmpv.",
                  "Hardware decoding, seeking, volume up to 150% and fullscreen, "
                  "from the buttons, the keyboard or the mouse wheel.", a, t, width=400)
    wx, wy, ww, wh = 520, 110, 700, 470
    k = prog(t, 0.2, 0.7)
    cr.save()
    cr.translate(0, 30 * (1 - k))
    window(cr, wx, wy, ww, wh, "qiptv — Player", a * k)
    # "Video": slowly moving abstract shapes standing in for a picture
    vx, vy, vw, vh = wx + 1, wy + 34, ww - 2, wh - 34 - 92
    paused = 6.4 <= t < 7.6
    vt = t if not paused else 6.4
    cr.save()
    cr.rectangle(vx, vy, vw, vh)
    cr.clip()
    g = cairo.LinearGradient(vx, vy, vx + vw, vy + vh)
    g.add_color_stop_rgba(0, 0.08, 0.20, 0.32, a * k)
    g.add_color_stop_rgba(1, 0.03, 0.06, 0.10, a * k)
    cr.set_source(g)
    cr.paint()
    for i in range(5):
        px = vx + vw * (0.5 + 0.38 * math.sin(vt * 0.5 + i * 1.7))
        py = vy + vh * (0.5 + 0.30 * math.cos(vt * 0.4 + i * 2.1))
        rg = cairo.RadialGradient(px, py, 0, px, py, 140)
        col = POSTER_COLORS[i]
        rg.add_color_stop_rgba(0, *col, 0.35 * a * k)
        rg.add_color_stop_rgba(1, *col, 0)
        cr.set_source(rg)
        cr.paint()
    cr.restore()
    # Control bar
    cy = wy + wh - 92
    text(cr, "Docu World — The Long Coast", wx + 18, cy + 26, 14, TEXT, a * k, bold=True)
    by = cy + 62
    bx = wx + 26
    for name in ("back", "prev", "rew", "pause" if not paused else "play", "stop", "ffwd", "next"):
        hl = (name == "ffwd" and 1.6 <= t < 2.3) or (name in ("pause", "play") and
                                                     (6.2 <= t < 6.6 or 7.4 <= t < 7.8))
        if hl:
            fill_rrect(cr, bx - 16, by - 16, 32, 32, 6, BLUE, 0.25 * a * k)
        icon(cr, name, bx, by, 14, BLUE if hl else TEXT, a * k)
        bx += 34
    # Seek slider: +10s jump at 1.8s and again at 2.8s
    duration = 52 * 60
    pos = 14 * 60 + 5 + t
    pos += 10 * prog(t, 1.8, 0.25) + 10 * prog(t, 2.8, 0.25)
    sx, sw = bx + 4, 140
    fill_rrect(cr, sx, by - 2, sw, 4, 2, LINE, a * k)
    fill_rrect(cr, sx, by - 2, sw * pos / duration, 4, 2, BLUE, a * k)
    cr.arc(sx + sw * pos / duration, by, 7, 0, 2 * math.pi)
    cr.set_source_rgba(*rgba(TEXT, a * k))
    cr.fill()
    p = int(pos)
    text(cr, f"{p // 60}:{p % 60:02d} / 52:00", sx + sw + 12, by + 5, 13, MUTED, a * k, mono=True)
    # Volume: wheel up 100 -> 130 between 3.8s and 5.4s, in 5% steps
    vol = 100 + 5 * int(6 * clamp((t - 3.8) / 1.6))
    vxs = sx + sw + 142
    icon(cr, "volume", vxs, by, 14, TEXT, a * k)
    fill_rrect(cr, vxs + 16, by - 2, 60, 4, 2, LINE, a * k)
    fill_rrect(cr, vxs + 16, by - 2, 60 * vol / 150, 4, 2, BLUE, a * k)
    text(cr, f"{vol}%", vxs + 84, by + 5, 13, BLUE if 3.8 <= t < 5.8 else MUTED, a * k, mono=True)
    # Toasts over the video
    for start, msg in ((1.8, "+10 s"), (2.8, "+10 s"), (4.0, f"Volume {vol}%"), (6.4, "Paused")):
        kk = clamp(1 - abs(t - start - 0.4) / 0.6) if msg != "Paused" else (
            clamp((t - 6.4) / 0.2) * clamp((7.6 - t) / 0.2))
        if msg.startswith("Volume"):
            kk = clamp((t - 3.8) / 0.2) * clamp((5.9 - t) / 0.3)
        if kk > 0:
            w = text_width(cr, msg, 20, bold=True) + 36
            fill_rrect(cr, vx + vw / 2 - w / 2, vy + 24, w, 42, 21, (0, 0, 0), 0.6 * a * k * kk)
            text(cr, msg, vx + vw / 2, vy + 52, 20, TEXT, a * k * kk, bold=True, align="center")
    cr.restore()
    # Keycaps under the copy, lighting up in sync
    ky = end + 26
    x = 80
    for label, t0 in (("→", 1.8), ("→", 2.8)):
        x += keycap(cr, label, x, ky, a * prog(t, 1.4, 0.4), clamp(1 - abs(t - t0) / 0.2)) + 10
    x += 14
    kk = prog(t, 3.5, 0.4)
    text(cr, "wheel", x, ky + 28, 16, BLUE if 3.8 <= t < 5.4 else MUTED, a * kk, mono=True)
    x += text_width(cr, "wheel", 16, mono=True) + 24
    keycap(cr, "Space", x, ky, a * prog(t, 5.9, 0.4), clamp(1 - abs(t - 6.3) / 0.2))


EPISODES = [("S1E8", "Low Tide"), ("S1E9", "The Lighthouse"), ("S1E10", "Storm Season"),
            ("S2E1", "New Harbour"), ("S2E2", "Northern Lights")]


def scene_playlist(cr, t, a):
    heading(cr, "04 — KEEP WATCHING", "Next follows the list you came from.",
            "Channels wrap around. Movies stay in their category. Episodes run straight "
            "on into the next season, and the next one starts when one finishes.", a, t, width=420)
    wx, wy, ww, wh = 560, 120, 640, 460
    k = prog(t, 0.2, 0.7)
    cr.save()
    cr.translate(0, 30 * (1 - k))
    window(cr, wx, wy, ww, wh, "qiptv — The Long Coast", a * k)
    # Cursor: S1E9 -> (N at 1.6s) S1E10 -> (auto-advance at 5.2s) S2E1
    cur = 1 + (t >= 1.6) + (t >= 5.2)
    rows_y = wy + 72
    for i, (code, name) in enumerate(EPISODES):
        y = rows_y + i * 70 + (22 if i >= 3 else 0)
        if i == 3:
            text(cr, "SEASON 2", wx + 30, y - 12, 12, AMBER, a * k, mono=True)
        elif i == 0:
            text(cr, "SEASON 1", wx + 30, y - 10 + 0, 12, MUTED, a * k, mono=True)
        active = i == cur
        fill_rrect(cr, wx + 20, y, ww - 40, 56, 8, PANEL_2 if active else PANEL, a * k)
        if active:
            stroke_rrect(cr, wx + 20.5, y + 0.5, ww - 41, 55, 8, BLUE, a * k, 1.5)
            icon(cr, "play", wx + 48, y + 28, 14, BLUE, a * k)
        text(cr, code, wx + 70, y + 35, 15, BLUE if active else MUTED, a * k, mono=True, bold=active)
        text(cr, name, wx + 150, y + 35, 17, TEXT if active else MUTED, a * k, bold=active)
        if active and i == 2:
            # progress of the playing episode racing to the end before auto-advance
            pr = 0.2 + 0.8 * clamp((t - 2.4) / 2.6)
            fill_rrect(cr, wx + 150, y + 44, ww - 220, 3, 1.5, LINE, a * k)
            fill_rrect(cr, wx + 150, y + 44, (ww - 220) * pr, 3, 1.5, BLUE, a * k)
    cr.restore()
    kk = prog(t, 1.2, 0.4)
    keycap(cr, "N", 80, 470, a * kk, clamp(1 - abs(t - 1.6) / 0.2))
    text(cr, "or", 140, 498, 16, MUTED, a * kk, mono=True)
    keycap(cr, "", 176, 470, a * kk, 0.0, w=60, glyph="next")
    text(cr, "media key", 250, 498, 16, MUTED, a * kk, mono=True)
    kk = clamp((t - 4.9) / 0.3) * clamp((7.4 - t) / 0.4)
    if kk > 0:
        w = 300
        fill_rrect(cr, 80, 540, w, 50, 25, AMBER, 0.15 * a * kk)
        stroke_rrect(cr, 80.5, 540.5, w - 1, 49, 25, AMBER, a * kk)
        text(cr, "Up next: S2E1 — New Harbour", 80 + w / 2, 571, 15, AMBER, a * kk, align="center")


def scene_desktop(cr, t, a):
    heading(cr, "05 — STAYS OUT OF THE WAY", "Tray, media keys, notifications.",
            "Turn on close-to-tray and qiptv waits in the system tray. While it has "
            "focus, your keyboard's media keys play, pause, stop and skip.", a, t, width=420)
    # Desktop panel along the bottom right
    k = prog(t, 0.2, 0.6)
    px, py, pw, ph = 600, 610, 620, 48
    fill_rrect(cr, px, py + 20 * (1 - k), pw, ph, 12, PANEL_2, a * k)
    for i in range(4):
        fill_rrect(cr, px + 18 + i * 44, py + 12 + 20 * (1 - k), 24, 24, 6, LINE, a * k)
    logo(cr, px + pw - 130, py + 24 + 20 * (1 - k), 34)
    text(cr, "21:30", px + pw - 22, py + 30 + 20 * (1 - k), 15, TEXT, a * k, align="right",
         mono=True)
    # Notification card
    kn = prog(t, 1.0, 0.6) * clamp((7.5 - t) / 0.5)
    nx, ny = 820, 410 + 20 * (1 - kn)
    fill_rrect(cr, nx, ny, 400, 120, 12, PANEL, a * kn)
    stroke_rrect(cr, nx + 0.5, ny + 0.5, 399, 119, 12, LINE, a * kn)
    logo(cr, nx + 44, ny + 60, 52)
    text(cr, "Now playing", nx + 84, ny + 50, 17, TEXT, a * kn, bold=True)
    text(cr, "Docu World — The Long Coast", nx + 84, ny + 78, 15, MUTED, a * kn)
    # Media keys
    x = 80
    for i, (glyph, t0) in enumerate((("prev", 3.0), ("play", 3.6), ("stop", 4.2), ("next", 4.8))):
        kk = prog(t, 2.4 + i * 0.12, 0.4)
        x += keycap(cr, "", x, 470, a * kk, clamp(1 - abs(t - t0) / 0.2), w=60, glyph=glyph) + 12


def scene_outro(cr, t, a):
    logo(cr, W / 2, 210, 120, t, waves=1.0)
    k = prog(t, 0.2, 0.7)
    text(cr, "qiptv", W / 2, 330 + 10 * (1 - k), 60, TEXT, a * k, bold=True, align="center")
    k = prog(t, 0.6, 0.7)
    text(cr, "Free and open source, MIT licensed", W / 2, 375, 22, MUTED, a * k, align="center")
    k = prog(t, 1.0, 0.7)
    cmd = "make deb && sudo apt install ./dist/qiptv_*.deb"
    w = text_width(cr, cmd, 18, mono=True) + 48
    fill_rrect(cr, W / 2 - w / 2, 420, w, 52, 10, PANEL, a * k)
    stroke_rrect(cr, W / 2 - w / 2 + 0.5, 420.5, w - 1, 51, 10, LINE, a * k)
    text(cr, cmd, W / 2, 452, 18, TEXT, a * k, mono=True, align="center")
    k = prog(t, 1.4, 0.7)
    text(cr, "github.com/xmonader/qiptv", W / 2, 530, 22, BLUE, a * k, bold=True, align="center")
    k = prog(t, 2.0, 0.7)
    text(cr, "For services you are entitled to use. qiptv ships no content.", W / 2, 640, 14,
         MUTED, a * k, align="center")


SCENES = [
    (5.0, scene_title),
    (7.0, scene_login),
    (8.0, scene_browse),
    (9.0, scene_player),
    (8.0, scene_playlist),
    (7.0, scene_desktop),
    (6.0, scene_outro),
]
FADE = 0.45
TOTAL = sum(d for d, _ in SCENES)


def draw_frame(cr, gt):
    background(cr, gt)
    start = 0.0
    for dur, fn in SCENES:
        if start <= gt < start + dur or (fn is SCENES[-1][1] and gt >= start):
            t = gt - start
            a = clamp(t / FADE) * clamp((dur - t) / FADE)
            if fn is SCENES[0][1]:
                a = clamp((dur - t) / FADE)  # first scene starts fully visible
            if fn is SCENES[-1][1]:
                a = clamp(t / FADE)          # last scene holds to the end
            cr.push_group()
            fn(cr, t, 1.0)
            cr.pop_group_to_source()
            cr.paint_with_alpha(a)
            return
        start += dur


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("out", help="output .mp4")
    ap.add_argument("--poster", help="also write a poster frame (JPEG) here")
    ap.add_argument("--poster-time", type=float, default=27.5)
    ap.add_argument("--preview", type=float, help="render only this many seconds")
    args = ap.parse_args()

    if not shutil.which("ffmpeg"):
        sys.exit("ffmpeg not found")

    surface = cairo.ImageSurface(cairo.FORMAT_RGB24, W, H)
    cr = cairo.Context(surface)
    cr.set_antialias(cairo.ANTIALIAS_BEST)

    total = min(TOTAL, args.preview) if args.preview else TOTAL
    frames = int(round(total * FPS))
    ff = subprocess.Popen(
        ["ffmpeg", "-loglevel", "error", "-y", "-f", "rawvideo", "-pix_fmt", "bgra",
         "-s", f"{W}x{H}", "-r", str(FPS), "-i", "-",
         "-c:v", "libx264", "-preset", "slow", "-crf", "22", "-pix_fmt", "yuv420p",
         "-tune", "animation", "-movflags", "+faststart", args.out],
        stdin=subprocess.PIPE)
    try:
        for f in range(frames):
            draw_frame(cr, f / FPS)
            surface.flush()
            ff.stdin.write(bytes(surface.get_data()))
    finally:
        ff.stdin.close()
    if ff.wait() != 0:
        sys.exit(f"ffmpeg failed with exit code {ff.returncode}")

    if args.poster:
        draw_frame(cr, args.poster_time)
        surface.flush()
        png = args.poster + ".png"
        surface.write_to_png(png)
        r = subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-i", png, "-q:v", "3",
                            args.poster])
        subprocess.run(["rm", "-f", png])
        if r.returncode != 0:
            sys.exit("poster conversion failed")
    print(f"wrote {args.out} ({frames} frames, {total:.1f}s)")


if __name__ == "__main__":
    main()
