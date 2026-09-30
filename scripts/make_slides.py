#!/usr/bin/env python3
"""Builds docs/SpeakerScope_Hackathon.pptx (16:9 dark deck) and converts it
to docs/SpeakerScope_Slides.pdf via PowerPoint COM automation."""
import os, sys
from pptx import Presentation
from pptx.util import Inches, Pt, Emu
from pptx.dml.color import RGBColor
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE

TEAL   = RGBColor(0x34, 0xC2, 0xDB)
BG     = RGBColor(0x0D, 0x0F, 0x13)
CARD   = RGBColor(0x15, 0x18, 0x1D)
CARD2  = RGBColor(0x1B, 0x1F, 0x23)
WHITE  = RGBColor(0xE8, 0xED, 0xF3)
MUTED  = RGBColor(0x8B, 0x94, 0x9E)
GREEN  = RGBColor(0x4A, 0xDA, 0x78)
AMBER  = RGBColor(0xFF, 0x9E, 0x45)
ORANGE = RGBColor(0xFF, 0x8C, 0x37)
SKY    = RGBColor(0xE5, 0xCC, 0xDB)
GREEN2 = RGBColor(0x00, 0x94, 0x59)
YELLOW = RGBColor(0xF0, 0xE6, 0x00)
BLUE   = RGBColor(0xE6, 0x56, 0x78)
VERM   = RGBColor(0xF0, 0xB0, 0x7A)
RPURP  = RGBColor(0x5B, 0x9E, 0xB4)
GREY   = RGBColor(0x55, 0x55, 0x55)
SPK    = [ORANGE, SKY, GREEN2, YELLOW, BLUE, VERM, RPURP, GREY]

HERE = os.path.dirname(os.path.abspath(__file__))
DOCS = os.path.join(HERE, "..", "docs")
os.makedirs(DOCS, exist_ok=True)
PPTX = os.path.join(DOCS, "SpeakerScope_Hackathon.pptx")
PDF  = os.path.join(DOCS, "SpeakerScope_Slides.pdf")

prs = Presentation()
prs.slide_width  = Inches(13.333)
prs.slide_height = Inches(7.5)
BLANK = prs.slide_layouts[6]

def slide():
    s = prs.slides.add_slide(BLANK)
    bg = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, 0, 0,
                            prs.slide_width, prs.slide_height)
    bg.fill.solid(); bg.fill.fore_color.rgb = BG
    bg.line.fill.background()
    bg.shadow.inherit = False
    return s

def txbox(s, l, t, w, h):
    tb = s.shapes.add_textbox(l, t, w, h)
    tf = tb.text_frame
    tf.word_wrap = True
    return tf

def text(tf, txt, size, color, bold=False, first=False, align=PP_ALIGN.LEFT,
         space_after=4, bullet=False, font="Segoe UI"):
    p = tf.paragraphs[0] if first else tf.add_paragraph()
    p.alignment = align
    p.space_after = Pt(space_after)
    r = p.add_run(); r.text = ("\u2022  " if bullet else "") + txt
    r.font.size = Pt(size); r.font.bold = bold
    r.font.color.rgb = color; r.font.name = font
    return p

def bar(s, l=Inches(0.55), t=Inches(0.5), w=Inches(0.09), h=Inches(0.75),
        color=TEAL):
    b = s.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, l, t, w, h)
    b.fill.solid(); b.fill.fore_color.rgb = color
    b.line.fill.background(); b.shadow.inherit = False

def title(s, txt, sub=None):
    bar(s)
    tf = txbox(s, Inches(0.85), Inches(0.42), Inches(11.8), Inches(1.0))
    text(tf, txt, 36, WHITE, bold=True, first=True)
    if sub:
        tf2 = txbox(s, Inches(0.87), Inches(1.22), Inches(11.5), Inches(0.5))
        text(tf2, sub, 15, MUTED, first=True)

def card(s, l, t, w, h, fill=CARD):
    c = s.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, l, t, w, h)
    c.fill.solid(); c.fill.fore_color.rgb = fill
    c.line.color.rgb = RGBColor(0x2E, 0x33, 0x3B); c.line.width = Pt(0.75)
    c.shadow.inherit = False
    return c

# ---------------------------------------------------------------- slide 1
s = slide()
tf = txbox(s, Inches(1.0), Inches(2.35), Inches(11.3), Inches(1.4))
text(tf, "SpeakerScope", 66, TEAL, bold=True, first=True, align=PP_ALIGN.CENTER)
tf = txbox(s, Inches(1.0), Inches(3.75), Inches(11.3), Inches(1.6))
text(tf, "Live speaker-attributed transcription", 24, WHITE, first=True,
     align=PP_ALIGN.CENTER)
text(tf, "who said what, as they say it", 16, MUTED, align=PP_ALIGN.CENTER)
tf = txbox(s, Inches(1.0), Inches(5.7), Inches(11.3), Inches(0.8))
text(tf, "AssemblyAI x lablab.ai Hackathon  |  September 2026", 14, MUTED,
     first=True, align=PP_ALIGN.CENTER)
text(tf, "AssemblyAI Universal Streaming  +  NVIDIA Nemotron-3-Diarization (on-device)",
     14, TEAL, align=PP_ALIGN.CENTER)
for i in range(8):  # speaker dot row
    d = s.shapes.add_shape(MSO_SHAPE.OVAL, Inches(5.9 + i*0.28), Inches(6.55),
                           Inches(0.14), Inches(0.14))
    d.fill.solid(); d.fill.fore_color.rgb = SPK[i]; d.line.fill.background()
    d.shadow.inherit = False

# ---------------------------------------------------------------- slide 2
s = slide()
title(s, "The Problem", "Transcripts know what was said - not who said it")
cards = [
    ("Flat transcripts", "No speaker labels: meetings become a wall of text - "
     "unusable for recaps, QA, or action items."),
    ("Cloud diarization", "Post-hoc upload, minutes of latency, per-minute "
     "billing, and raw audio leaves the device."),
    ("Real-time need", "Live meeting assist, call coaching, accessibility: "
     "labels must appear while people speak."),
]
for i, (h, b) in enumerate(cards):
    x = Inches(0.75 + i * 4.15)
    card(s, x, Inches(2.1), Inches(3.85), Inches(3.1))
    tf = txbox(s, x + Inches(0.3), Inches(2.4), Inches(3.25), Inches(2.6))
    text(tf, h, 19, TEAL, bold=True, first=True, space_after=10)
    text(tf, b, 14, WHITE)
tf = txbox(s, Inches(0.85), Inches(5.7), Inches(11.6), Inches(1.0))
text(tf, "Gap in the market: fast, private, speaker-attributed transcription "
         "running on commodity desktop hardware.", 15, AMBER, bold=True,
     first=True)

# ---------------------------------------------------------------- slide 3
s = slide()
title(s, "The Solution", "Two best-in-class engines fused in one native C++ app")
card(s, Inches(0.75), Inches(2.0), Inches(5.9), Inches(3.4))
tf = txbox(s, Inches(1.05), Inches(2.3), Inches(5.3), Inches(2.9))
text(tf, "WHAT was said", 13, MUTED, bold=True, first=True)
text(tf, "AssemblyAI Universal Streaming", 20, TEAL, bold=True, space_after=8)
for b in ["Real-time WebSocket (v3), 16 kHz PCM",
          "Word-level timestamps, sub-second partials",
          "Clean session lifecycle - billing-safe"]:
    text(tf, b, 13.5, WHITE, bullet=True)
card(s, Inches(6.85), Inches(2.0), Inches(5.9), Inches(3.4))
tf = txbox(s, Inches(7.15), Inches(2.3), Inches(5.3), Inches(2.9))
text(tf, "WHO is speaking", 13, MUTED, bold=True, first=True)
text(tf, "NVIDIA Nemotron-3-Diarization", 20, TEAL, bold=True, space_after=8)
for b in ["100M-param model on-device (GGUF q8_0)",
          "Native C++ via audio.cpp - no GPU, no Python",
          "8 channels, overlap-aware, streaming"]:
    text(tf, b, 13.5, WHITE, bullet=True)
tf = txbox(s, Inches(0.85), Inches(5.75), Inches(11.6), Inches(1.0))
text(tf, "Timestamp-domain fusion -> every word gets a speaker label live, "
         "shown as Person 1..8 lanes users can rename, filter and export.",
     15, WHITE, bold=True, first=True)

# ---------------------------------------------------------------- slide 4
s = slide()
title(s, "Architecture", "One capture, two consumers, one fused transcript")
steps = [
    ("AUDIO", "mic / WASAPI loopback / WAV  ->  16 kHz mono lock-free ring"),
    ("STT", "AssemblyAI v3 WS  ->  50 ms pcm_s16le  ->  turns + word timestamps"),
    ("DIAR", "audio.cpp Nemotron-3  ->  streaming inference  ->  open speaker turns"),
    ("FUSE", "word-midpoint argmax + hysteresis + overlap flag"),
    ("UI", "Dear ImGui - live lanes, rename people, filter/copy/export per person"),
]
y = 1.9
for i, (k, v) in enumerate(steps):
    card(s, Inches(1.3), Inches(y), Inches(10.7), Inches(0.82),
         CARD if i % 2 == 0 else CARD2)
    tf = txbox(s, Inches(1.55), Inches(y + 0.13), Inches(10.2), Inches(0.6))
    text(tf, k, 14, TEAL, bold=True, first=True)
    p = tf.paragraphs[0]
    r = p.add_run(); r.text = "   " + v
    r.font.size = Pt(13.5); r.font.color.rgb = WHITE; r.font.name = "Segoe UI"
    if i < len(steps) - 1:
        ar = txbox(s, Inches(6.4), Inches(y + 0.80), Inches(0.6), Inches(0.3))
        text(ar, "v", 12, MUTED, bold=True, first=True, align=PP_ALIGN.CENTER)
    y += 1.02
tf = txbox(s, Inches(0.85), Inches(7.0 - 0.55), Inches(11.6), Inches(0.5))

# ---------------------------------------------------------------- slide 5
s = slide()
title(s, "Engineering highlights")
items = [
    ("Open-turn streaming patch", "audio.cpp decoder patched to emit live "
     "open turns - speakers labeled while talking, not only after pauses."),
    ("7.6x realtime on CPU", "Custom chunk geometry (chunk_len=40) vs. 0.53x "
     "for the stock 'low' profile - identical accuracy, ~5 s label granularity."),
    ("Tee architecture", "STT and diarization consume separate rings - local "
     "inference can never starve the realtime AssemblyAI stream."),
    ("Beyond 8 speakers", "8-channel limit is concurrency-only; voice-embedding "
     "clustering resolves unlimited global Person identities."),
    ("Privacy", "Diarization fully on-device; only transcription audio goes "
     "to AssemblyAI."),
]
for i, (h, b) in enumerate(items):
    col = i % 2; row = i // 2
    x = Inches(0.75 + col * 6.1); y = Inches(1.75 + row * 1.72)
    w = Inches(5.85 if i < 4 else 11.95)
    if i == 4: x = Inches(0.75)
    card(s, x, y, w, Inches(1.5))
    tf = txbox(s, x + Inches(0.28), y + Inches(0.16), w - Inches(0.56),
               Inches(1.25))
    text(tf, h, 16, TEAL, bold=True, first=True, space_after=5)
    text(tf, b, 12.5, WHITE)

# ---------------------------------------------------------------- slide 6
s = slide()
title(s, "Verified results", "It runs - headless and in the GUI")
stats = [("117/117", "automated tests green - incl. 80-case generated-meeting battery"),
         ("55.8 s", "4-voice wav -> live Nemotron + AssemblyAI -> attributed transcript"),
         ("7.6x RT", "on-device diarization throughput on a 12-core CPU"),
         ("<1 s", "session stop - STT socket closed before diar drain (billing-safe)")]
for i, (big, small) in enumerate(stats):
    x = Inches(0.75 + i * 3.1)
    card(s, x, Inches(1.9), Inches(2.9), Inches(2.0))
    tf = txbox(s, x + Inches(0.22), Inches(2.15), Inches(2.5), Inches(1.6))
    text(tf, big, 30, TEAL, bold=True, first=True)
    text(tf, small, 11.5, WHITE)
tf = txbox(s, Inches(0.85), Inches(4.35), Inches(11.7), Inches(2.5))
text(tf, "Demo artifacts", 15, MUTED, bold=True, first=True, space_after=8)
for b in [
    "speakerscope.exe - GUI: live lanes, rename Person N, filter & export per person",
    "speakerscope_sim.exe - headless end-to-end: wav -> real AssemblyAI + Nemotron -> fused transcript",
    "speakerscope_diar_bench.exe - model throughput benchmark (16x RT max profile)",
    "Sample output: '[Person 2] The diarization model now supports up to 8 simultaneous speakers.'",
]:
    text(tf, b, 14, WHITE, bullet=True)

# ---------------------------------------------------------------- slide 7
s = slide()
title(s, "Business value & stack")
tf = txbox(s, Inches(0.85), Inches(1.8), Inches(11.6), Inches(2.4))
text(tf, "A transcript tells you what was said. A record tells you who said it.",
     20, WHITE, bold=True, first=True, space_after=12)
for b in [
    "Hybrid teams - searchable, attributable meeting recaps",
    "Call centers - agent/customer separation for QA and coaching",
    "Accessibility - follow conversations by speaker in real time",
    "Privacy-sensitive settings - diarization never leaves the device",
]:
    text(tf, b, 15, WHITE, bullet=True)
card(s, Inches(0.85), Inches(4.5), Inches(11.6), Inches(1.9))
tf = txbox(s, Inches(1.15), Inches(4.75), Inches(11.0), Inches(1.5))
text(tf, "Stack", 15, TEAL, bold=True, first=True, space_after=6)
text(tf, "C++20 | CMake + vcpkg | Dear ImGui + GLFW | miniaudio | "
         "IXWebSocket + mbedTLS | AssemblyAI Streaming v3 | audio.cpp (GGUF) | "
         "Nemotron-3-Diarization q8_0 | GoogleTest | Windows, MIT license",
     13.5, WHITE)
tf = txbox(s, Inches(0.85), Inches(6.6), Inches(11.6), Inches(0.6))
text(tf, "SpeakerScope - live, on-device, speaker-attributed transcription.",
     15, TEAL, bold=True, first=True, align=PP_ALIGN.CENTER)

prs.save(PPTX)
print("wrote", os.path.abspath(PPTX))

# ---------------------------------------------------------------- to PDF
import win32com.client  # type: ignore
app = win32com.client.Dispatch("PowerPoint.Application")
try:
    pres = app.Presentations.Open(os.path.abspath(PPTX), WithWindow=False)
    pres.SaveAs(os.path.abspath(PDF), 32)  # 32 = ppSaveAsPDF
    pres.Close()
finally:
    app.Quit()
print("wrote", os.path.abspath(PDF))
