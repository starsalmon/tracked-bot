from reportlab.lib.pagesizes import A4, landscape
from reportlab.pdfgen import canvas
from reportlab.platypus import Table, TableStyle, Paragraph
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.colors import HexColor

W, H = landscape(A4)
M = 24
c = canvas.Canvas("/workspace/tracker_pinmap.pdf", pagesize=landscape(A4))
c.setTitle("Tracker bot (tracked-bot) pinmap - UM TinyS3[D]")

NAVY = HexColor("#1f2d3d"); GREY = HexColor("#f2f4f7"); LINE = HexColor("#b8c0cc")
C_MOT = HexColor("#fde2c8"); C_SEN = HexColor("#d6ecff"); C_IR = HexColor("#e6dcff")
C_I2C = HexColor("#d4f1dc"); C_ONB = HexColor("#e5e7eb"); C_FREE = HexColor("#fff6bf"); C_OUT = HexColor("#ffd9d9")

# Title
c.setFillColor(NAVY); c.setFont("Helvetica-Bold", 19)
c.drawString(M, H - M - 14, "Tracker bot (tracked-bot, /bot2) \u2013 pinmap")
c.setFont("Helvetica", 9.5); c.setFillColor(HexColor("#444444"))
c.drawString(M, H - M - 28, "Unexpected Maker TinyS3[D] (ESP32-S3)  |  DRV8833 tracks, 130 motors  |  1S 800 mAh LiPo + boost converter  |  firmware env tracked_s3_usb / tracked_s3_ota")
c.setStrokeColor(NAVY); c.setLineWidth(1.2); c.line(M, H - M - 34, W - M, H - M - 34)

cell = ParagraphStyle("c", fontName="Helvetica", fontSize=7.6, leading=9)
cellb = ParagraphStyle("cb", parent=cell, fontName="Helvetica-Bold")
hdr = ParagraphStyle("h", parent=cell, fontName="Helvetica-Bold", textColor=colors.white)
def P(t, s=cell): return Paragraph(t, s)

def section(x, y, text):
    c.setFont("Helvetica-Bold", 10); c.setFillColor(NAVY); c.drawString(x, y, text)

# ---- (1) GPIO table
rows = [
 ("0",  "Left motor IN2", "DRV8833 AIN2", "pos 11. BOOT strap, pulled high at reset (firmware holds it LOW first)", C_MOT),
 ("21", "Left motor IN1", "DRV8833 AIN1", "pos 10", C_MOT),
 ("44", "Right motor IN1", "DRV8833 BIN1", "pos 12. UART0 RX. Right track direction inverted in firmware", C_MOT),
 ("43", "Right motor IN2", "DRV8833 BIN2", "pos 13. UART0 TX, boot log toggles it", C_MOT),
 ("4",  "Solar servo signal", "Panel tilt servo", "pos 8. 45\u2013125\u00b0, 90 = flat/centre. Signal released 600 ms after each move", C_SEN),
 ("6",  "Side IR left", "IR proximity, left", "pos 16. Digital, LOW = object (~5 cm), pull-up", C_SEN),
 ("5",  "Side IR right", "IR proximity, right", "pos 9. Digital, LOW = object (~5 cm), pull-up", C_SEN),
 ("7",  "Speaker", "Passive piezo", "pos 17. LEDC PWM", C_SEN),
 ("34", "IR TX (fleet ID)", "IR LED + resistor", "pos 20. 38 kHz modulated beacon", C_IR),
 ("3",  "IR RX (fleet ID)", "TSOP 38 kHz demod", "pos 7. LOW = carrier seen. JTAG-source strap, input only", C_IR),
 ("8",  "I2C SDA", "IMU, ADS1115, fuel gauge", "pos 18. Board default SDA", C_I2C),
 ("9",  "I2C SCL", "IMU, ADS1115, fuel gauge", "pos 19. Board default SCL", C_I2C),
 ("17", "RGB LED power", "Onboard", "Firmware drives it; off with the LED", C_ONB),
 ("18", "RGB LED data", "Onboard NeoPixel", "Dim blue idle, green moving, amber OTA", C_ONB),
 ("33", "5 V (VBUS) sense", "Onboard", "Digital, HIGH = USB/boost 5 V present. Not ADC", C_ONB),
 ("10", "Unused", "Nothing connected", "No battery divider on this board (onboard fuel gauge instead). Unassigned in firmware", C_ONB),
 ("1, 2, 35, 36, 37", "Free", "Nothing connected", "Unassigned in firmware", C_FREE),
]
data = [[P("GPIO", hdr), P("Function", hdr), P("Connected part / wire", hdr), P("Notes", hdr)]]
for g, f, p, n, _ in rows:
    data.append([P(g, cellb), P(f, cellb), P(p), P(n)])
tw = [58, 108, 112, 198]
t = Table(data, colWidths=tw, rowHeights=[15] + [None]*len(rows))
st = [("BACKGROUND", (0,0), (-1,0), NAVY), ("VALIGN", (0,0), (-1,-1), "MIDDLE"),
      ("GRID", (0,0), (-1,-1), 0.4, LINE), ("TOPPADDING", (0,0), (-1,-1), 1.6), ("BOTTOMPADDING", (0,0), (-1,-1), 1.6),
      ("LEFTPADDING", (0,0), (-1,-1), 3), ("RIGHTPADDING", (0,0), (-1,-1), 3)]
for i, r in enumerate(rows, start=1):
    st.append(("BACKGROUND", (0,i), (0,i), r[4]))
t.setStyle(TableStyle(st))
section(M, H - M - 50, "1. GPIO table (sorted by function group)")
tw_, th_ = t.wrap(0, 0)
t.drawOn(c, M, H - M - 56 - th_)
table_bottom = H - M - 56 - th_

# ---- (2) grouped layout diagram
dy_top = table_bottom - 20
section(M, dy_top, "2. Board layout \u2013 pins grouped by use (not physical position)")
bx, bw = M + 150, 170
by_top = dy_top - 8
bh = by_top - M - 4
by = M + 4
c.setFillColor(HexColor("#2b3a4a")); c.setStrokeColor(NAVY); c.roundRect(bx, by, bw, bh, 6, fill=1)
c.setFillColor(colors.white); c.setFont("Helvetica-Bold", 10); c.drawCentredString(bx + bw/2, by + bh/2 + 4, "TinyS3[D]")
c.setFont("Helvetica", 7.5); c.drawCentredString(bx + bw/2, by + bh/2 - 7, "ESP32-S3")
c.setFillColor(HexColor("#d4f1dc")); c.roundRect(bx + 12, by + bh/2 - 52, bw - 24, 30, 3, fill=1, stroke=0)
c.setFillColor(colors.black); c.setFont("Helvetica-Bold", 7.4); c.drawCentredString(bx + bw/2, by + bh/2 - 36, "Fuel gauge MAX17048, I2C 0x36")
c.setFont("Helvetica", 6.8); c.drawCentredString(bx + bw/2, by + bh/2 - 46, "internal, on SDA 8 / SCL 9")
# USB tab
c.setFillColor(HexColor("#9aa5b1")); c.rect(bx + bw/2 - 14, by + bh - 3, 28, 8, fill=1, stroke=0)

def chip(x, y, w, h, fill, l1, l2=None, align="L"):
    c.setFillColor(fill); c.setStrokeColor(LINE); c.setLineWidth(0.5); c.roundRect(x, y, w, h, 2, fill=1)
    c.setFillColor(colors.black); c.setFont("Helvetica-Bold", 7.4); c.drawString(x + 3, y + h - 8.2, l1)
    if l2:
        c.setFont("Helvetica", 6.8); c.drawString(x + 3, y + 3, l2)
def wire(x1, y1, x2, y2):
    c.setStrokeColor(HexColor("#6b7686")); c.setLineWidth(0.7); c.line(x1, y1, x2, y2)

# left side: drive + power-ish
lw, lh = 140, 20
lx = M
left = [("L track: GPIO 21 / 0", "AIN1 / AIN2", C_MOT), ("R track: GPIO 44 / 43", "BIN1 / BIN2", C_MOT),
        ("Solar servo: GPIO 4", "45\u2013125\u00b0", C_SEN), ("Speaker: GPIO 7", "piezo", C_SEN)]
gap = (bh - 8 - len(left)*lh) / max(len(left)-1, 1)
for i, (a, b, col) in enumerate(left):
    y = by + bh - 6 - lh - i*(lh + gap)
    chip(lx, y, lw, lh, col, a, b); wire(lx + lw, y + lh/2, bx, y + lh/2)
# right side
rx = bx + bw + 12; rw = M + 476 - rx
right = [("I2C: SDA 8 / SCL 9", "IMU, ADS1115, fuel gauge", C_I2C), ("IR TX 34 / RX 3", "fleet ID", C_IR),
         ("Side IR: L 6 / R 5", "LOW = hit", C_SEN), ("Onboard: 17, 18, 33", "RGB power/data, 5 V sense", C_ONB)]
gap = (bh - 8 - len(right)*lh) / max(len(right)-1, 1)
for i, (a, b, col) in enumerate(right):
    y = by + bh - 6 - lh - i*(lh + gap)
    chip(rx, y, rw, lh, col, a, b); wire(bx + bw, y + lh/2, rx, y + lh/2)
# free
c.setFillColor(C_FREE); c.setStrokeColor(LINE); c.roundRect(bx + 14, by + 8, bw - 28, 22, 2, fill=1)
c.setFillColor(colors.black); c.setFont("Helvetica-Bold", 7.4); c.drawCentredString(bx + bw/2, by + 20, "FREE: GPIO 1, 2, 35, 36, 37")
c.setFont("Helvetica", 6.8); c.drawCentredString(bx + bw/2, by + 12, "GPIO 10 unused (no divider)")

# ---- right column
rx0 = M + 476 + 18
rw0 = W - M - rx0
y = H - M - 50
section(rx0, y, "3. I2C bus (SDA GPIO 8, SCL GPIO 9, 3.3 V)")
i2c = [[P("Device", hdr), P("Addr", hdr), P("Notes", hdr)],
       [P("MPU-6050 IMU", cellb), P("0x68"), P("AD0 to GND, INT unused")],
       [P("MAX17048 fuel gauge", cellb), P("0x36"), P("Onboard LiPo gauge, same bus. Firmware reads VCELL (0x02) and SOC (0x04) at ~2.5 Hz")],
       [P("ADS1115 ADC", cellb), P("0x48"), P("Auto-probes 0x48\u20130x4B. Gain \u00b14.096 V")],
       [P("SSD1306 OLED"), P("0x3C"), P("Optional, probed at boot")],
       [P("BH1750 light"), P("0x23"), P("Optional, probed at boot")],
       [P("VL53L0X ToF"), P("0x29"), P("Optional, probed at boot")]]
t2 = Table(i2c, colWidths=[88, 38, rw0 - 126])
t2.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,0), NAVY), ("GRID", (0,0), (-1,-1), 0.4, LINE),
    ("VALIGN", (0,0), (-1,-1), "MIDDLE"), ("TOPPADDING", (0,0), (-1,-1), 1.8), ("BOTTOMPADDING", (0,0), (-1,-1), 1.8),
    ("LEFTPADDING", (0,0), (-1,-1), 3), ("RIGHTPADDING", (0,0), (-1,-1), 3)]))
_, h2 = t2.wrap(0, 0); t2.drawOn(c, rx0, y - 6 - h2); y = y - 6 - h2 - 16

section(rx0, y, "4. ADS1115 channels")
ads = [[P("Ch", hdr), P("Signal", hdr), P("Front end", hdr), P("Ratio", hdr)],
       [P("A0", cellb), P("Left BPW34"), P("4.7k to GND, no divider"), P("\u00d71 (pin V)")],
       [P("A1", cellb), P("Right BPW34"), P("4.7k to GND, no divider"), P("\u00d71 (pin V)")],
       [P("A2", cellb), P("Main battery (1S LiPo)"), P("100k top / 100k bottom"), P("\u00d72.0")],
       [P("A3", cellb), P("Solar panel (6 V)"), P("100k from panel+ to pin, 47k pin to GND"), P("\u00d73.128")]]
t3 = Table(ads, colWidths=[22, 82, 88, rw0 - 192])
t3.setStyle(TableStyle([("BACKGROUND", (0,0), (-1,0), NAVY), ("GRID", (0,0), (-1,-1), 0.4, LINE),
    ("VALIGN", (0,0), (-1,-1), "MIDDLE"), ("TOPPADDING", (0,0), (-1,-1), 1.8), ("BOTTOMPADDING", (0,0), (-1,-1), 1.8),
    ("LEFTPADDING", (0,0), (-1,-1), 3), ("RIGHTPADDING", (0,0), (-1,-1), 3)]))
_, h3 = t3.wrap(0, 0); t3.drawOn(c, rx0, y - 6 - h3); y = y - 6 - h3 - 16

section(rx0, y, "5. Drive and power")
notes = [
 "<b>DRV8833:</b> nSLEEP and logic VCC to 3.3 V, VM to boost 5 V, GND common.",
 "<b>Track PWM:</b> 30 kHz, 8-bit, dead-band floor 150/255.",
 "<b>Power:</b> 1S 800 mAh LiPo into the boost converter; 5 V feeds the TinyS3 5 V pin and DRV8833 VM.",
 "<b>Battery gauge:</b> onboard MAX17048 (0x36), read by firmware. Cell volts on <font name='Courier'>/bot2/battery/voltage</font>, state of charge % on <font name='Courier'>/bot2/battery/percent</font>. Needs a LiPo connected. Battery is also on ADS1115 A2 (<font name='Courier'>/bot2/power/battery_v</font>) once wired.",
 "<b>Solar panel:</b> 6 V, 100k from panel+ to ADS A3, 47k from A3 to GND (not wired yet). Topic <font name='Courier'>/bot2/power/solar_v</font>.",
 "<b>ADS1115 absent:</b> firmware logs once at boot and publishes no ADS topics.",
 "<b>Servo:</b> boots 45 \u2192 125 \u2192 90, then the signal is released so it draws no idle current.",
 "<b>Strap pins:</b> GPIO 0 (BOOT), 3 (JTAG), 43/44 (UART0) carry boot-time duties; the motor lines are held LOW first in setup().",
]
ns = ParagraphStyle("n", parent=cell, fontSize=7.8, leading=10, spaceAfter=2.5)
yy = y - 4
for n in notes:
    p = Paragraph("\u2022 " + n, ns, ); w_, h_ = p.wrap(rw0, 200); p.drawOn(c, rx0, yy - h_); yy -= h_ + 2.5

# legend
ly = M + 4
c.setFont("Helvetica", 7); 
lx0 = rx0
for lab, col in [("Drive", C_MOT), ("Sensors/actuators", C_SEN), ("IR comms", C_IR), ("I2C", C_I2C), ("Onboard", C_ONB), ("Free", C_FREE)]:
    c.setFillColor(col); c.setStrokeColor(LINE); c.rect(lx0, ly, 8, 8, fill=1)
    c.setFillColor(colors.black); c.drawString(lx0 + 11, ly + 1.5, lab)
    lx0 += 11 + c.stringWidth(lab, "Helvetica", 7) + 8
c.setFont("Helvetica", 6.5); c.setFillColor(HexColor("#666666"))
c.drawString(rx0, ly + 12, "Source: include/tracked_pins_s3.h, include/tracked_pins.h, src/tracked_main.cpp")
c.showPage(); c.save()
