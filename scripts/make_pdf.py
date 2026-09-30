#!/usr/bin/env python3
"""Generates docs/SpeakerScope_Hackathon.pdf - hackathon submission doc."""
import os
from reportlab.lib.pagesizes import LETTER
from reportlab.lib.units import inch
from reportlab.lib.colors import HexColor, white
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT
from reportlab.platypus import (SimpleDocTemplate, Paragraph, Spacer, Table,
                                TableStyle, HRFlowable)

TEAL   = HexColor("#34C2DB")
DARK   = HexColor("#10151C")
SLATE  = HexColor("#44515E")
MUTED  = HexColor("#6B7A88")
PAPER  = HexColor("#F4F7F9")
CARD   = HexColor("#EAF4F7")
AMBER  = HexColor("#B8641B")

OUT = os.path.join(os.path.dirname(__file__), "..", "docs",
                   "SpeakerScope_Hackathon.pdf")

title  = ParagraphStyle("title", fontName="Helvetica-Bold", fontSize=26,
                        textColor=DARK, spaceAfter=2)
sub    = ParagraphStyle("sub", fontName="Helvetica", fontSize=11,
                        textColor=MUTED, spaceAfter=10)
h      = ParagraphStyle("h", fontName="Helvetica-Bold", fontSize=13,
                        textColor=TEAL, spaceBefore=12, spaceAfter=4)
body   = ParagraphStyle("body", fontName="Helvetica", fontSize=10.5,
                        textColor=DARK, leading=15, spaceAfter=5)
bullet = ParagraphStyle("bullet", parent=body, leftIndent=14,
                        bulletIndent=4, spaceAfter=3)
card   = ParagraphStyle("card", parent=body, textColor=DARK, spaceAfter=0)
mono   = ParagraphStyle("mono", fontName="Courier", fontSize=8.8,
                        textColor=SLATE, leading=12, spaceAfter=0)

def P(t, s=body): return Paragraph(t, s)
def B(t): return Paragraph(t, bullet, bulletText="\u2022")

doc = SimpleDocTemplate(OUT, pagesize=LETTER,
                        leftMargin=0.75*inch, rightMargin=0.75*inch,
                        topMargin=0.7*inch, bottomMargin=0.7*inch,
                        title="SpeakerScope - AssemblyAI Hackathon",
                        author="SpeakerScope Team")

E = []
E.append(P("SpeakerScope", title))
E.append(P("Live speaker-attributed transcription - native C++ desktop app<br/>"
           "AssemblyAI \u00d7 lablab.ai Hackathon - September 2026", sub))
E.append(HRFlowable(width="100%", thickness=1.4, color=TEAL, spaceAfter=8))

E.append(P("The Problem", h))
E.append(P(
    "Meeting transcription answers <i>what</i> was said but not <i>who said it</i>. "
    "Cloud diarization services exist, but they are slow (minutes after upload), "
    "cost per minute, and send raw audio off the device. Real-time use cases - "
    "live meeting notes, call-center assistance, accessibility - need speaker "
    "labels <b>as people speak</b>, on commodity hardware."))

E.append(P("The Solution", h))
E.append(P(
    "SpeakerScope is a Windows desktop application (C++20) that fuses two "
    "best-in-class engines into one live, color-coded transcript:"))
E.append(B("<b>AssemblyAI Universal streaming</b> - real-time speech-to-text "
           "over WebSocket (16 kHz PCM, word-level timestamps, sub-second "
           "partials). This is <i>what was said</i>."))
E.append(B("<b>NVIDIA Nemotron-3-Diarization on-device</b> - the 100M-param "
           "model running locally in native C++ via the audio.cpp GGUF runtime "
           "(q8_0, no GPU, no Python). This is <i>who is speaking when</i>."))
E.append(P(
    "A custom alignment engine fuses both streams in the timestamp domain: "
    "every AssemblyAI word is assigned to the speaker channel active at its "
    "midpoint, producing a live, speaker-colored transcript with "
    "<b>Person 1...Person 8</b> lanes the user can rename, filter, and export."))

E.append(P("Architecture", h))
pipe = [
    ["AUDIO", "mic / WASAPI loopback / WAV -> 16 kHz mono ring buffer (SPSC, lock-free)"],
    ["STT", "AssemblyAI v3 WebSocket -> 50 ms pcm_s16le frames -> partial+final turns, word timestamps"],
    ["DIAR", "audio.cpp Nemotron-3 -> streaming inference -> open speaker turns, 8 arrival-order channels"],
    ["FUSE", "word-midpoint argmax + hysteresis + overlap flag -> attributed segments"],
    ["UI", "Dear ImGui -> live speaker lanes, editable names, per-person filter/copy/export"],
]
t = Table([[P(a, ParagraphStyle("k", fontName="Helvetica-Bold", fontSize=9,
                                textColor=TEAL)),
            P(b, ParagraphStyle("v", fontName="Helvetica", fontSize=9.5,
                                textColor=DARK, leading=13))]
           for a, b in pipe],
          colWidths=[0.85*inch, 6.15*inch])
t.setStyle(TableStyle([
    ("BACKGROUND", (0,0), (-1,-1), CARD),
    ("ROWBACKGROUNDS", (0,0), (-1,-1), [CARD, PAPER]),
    ("TOPPADDING", (0,0), (-1,-1), 5), ("BOTTOMPADDING", (0,0), (-1,-1), 5),
    ("LEFTPADDING", (0,0), (-1,-1), 8), ("VALIGN", (0,0), (-1,-1), "TOP"),
    ("BOX", (0,0), (-1,-1), 0.75, HexColor("#BFDCE4")),
    ("LINEBELOW", (0,0), (-1,-2), 0.4, HexColor("#D5E6EB")),
]))
E.append(t)

E.append(P("Engineering highlights", h))
E.append(B("<b>Live open-turn streaming</b> - patched the audio.cpp decoder to "
           "emit open (still-growing) turns mid-stream, so speakers are labeled "
           "while they talk instead of only after they pause."))
E.append(B("<b>Tuned streaming profile</b> - custom chunk geometry "
           "(chunk_len=40) runs <b>7.6\u00d7 realtime</b> on a 12-core CPU "
           "vs. 0.53\u00d7 for the default 'low' profile, with identical "
           "accuracy - no backlog, ~5 s label granularity."))
E.append(B("<b>Tee architecture</b> - STT and diarization consume separate "
           "rings; slow local inference can never starve the realtime "
           "AssemblyAI stream."))
E.append(B("<b>Beyond 8 speakers</b> - Nemotron\u2019s 8-channel limit is a "
           "concurrency limit, not a total. A GlobalSpeakerResolver layer "
           "(voice embeddings + online centroid clustering) maps recycled "
           "channels to unlimited global Person identities."))
E.append(B("<b>Privacy story</b> - diarization is fully on-device; only audio "
           "needed for transcription goes to AssemblyAI."))

E.append(P("Verified results", h))
E.append(B("117/117 automated tests green - incl. an 80-case generated-meeting "
           "battery over the turn timeline + attributor."))
E.append(B("End-to-end simulation: 55.8 s multi-voice wav -> live Nemotron "
           "turns + real AssemblyAI streaming -> correctly attributed, "
           "overlap-flagged transcript (speakerscope_sim.exe)."))
E.append(B("Headless benchmark tool (speakerscope_diar_bench.exe) measures "
           "model throughput without burning API calls."))

E.append(P("Business value", h))
E.append(P(
    "Meeting notes that say <i>who</i> said <i>what</i> are the difference "
    "between a transcript and a record. Target users: hybrid teams (meeting "
    "recaps), call centers (agent/customer separation for QA), accessibility "
    "tools, and interview/research workflows. Because diarization runs "
    "on-device, the product works in privacy-sensitive settings where raw "
    "audio cannot leave the machine - and AssemblyAI\u2019s streaming STT "
    "keeps the words best-in-class."))

E.append(P("Stack", h))
E.append(P("C++20 \u00b7 CMake/vcpkg \u00b7 Dear ImGui + GLFW \u00b7 miniaudio \u00b7 "
           "IXWebSocket + mbedTLS \u00b7 AssemblyAI Streaming v3 \u00b7 audio.cpp "
           "(GGUF) \u00b7 Nemotron-3-Diarization q8_0 \u00b7 GoogleTest \u00b7 "
           "Windows 10/11, MIT license", body))

os.makedirs(os.path.dirname(OUT), exist_ok=True)
doc.build(E)
print("wrote", os.path.abspath(OUT))
