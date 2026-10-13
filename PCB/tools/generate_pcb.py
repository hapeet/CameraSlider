# -*- coding: utf-8 -*-
# Generator KiCad 9 projektu CameraSlider (schéma + knihovny + PCB)
# Spouštět KiCad pythonem:  "C:/Program Files/KiCad/9.0/bin/python.exe" gen.py
import os, uuid, math
import pcbnew

PRJ = "C:/LocalProjects/CameraSlider/PCB/CameraSlider"
NAME = "CameraSlider"
STOCK = "C:/Program Files/KiCad/9.0/share/kicad/footprints/"
ROOT_UUID = "e4f91eef-6134-4ed6-9ecb-3f0656d414ba"

def U():
    return str(uuid.uuid4())

# ============================================================================
#  NETLIST  (ref -> {pin: net}),  None = nezapojeno
# ============================================================================
ESP_J1 = ["3V3", "3V3", "RST", "IO4", "IO5", "IO6", "IO7", "IO15", "IO16", "IO17", "IO18",
          "IO8", "IO3", "IO46", "IO9", "IO10", "IO11", "IO12", "IO13", "IO14", "5V", "GND"]
ESP_J3 = ["GND", "TX", "RX", "IO1", "IO2", "IO42", "IO41", "IO40", "IO39", "IO38", "IO37",
          "IO36", "IO35", "IO0", "IO45", "IO48", "IO47", "IO21", "IO20", "IO19", "GND", "GND"]
ESP_NETS = {1: "+3V3", 2: "+3V3", 4: "STEP", 5: "DIR", 6: "EN", 10: "TMC_TX", 11: "TMC_UART",
            16: "HALL", 17: "RST_JMP", 21: "+5V", 22: "GND", 23: "GND", 43: "GND", 44: "GND"}
TMC_PINS = ["EN", "MS1", "MS2", "PDN", "UART", "CLK", "STEP", "DIR",
            "GND", "VIO", "1B", "1A", "2A", "2B", "GND", "VM"]

NETS = {
    "U1": {i: ESP_NETS.get(i) for i in range(1, 45)},
    "U2": {1: "EN", 2: "GND", 3: "GND", 4: "PDN4", 5: "PDN5", 6: None, 7: "STEP", 8: "DIR",
           9: "GND", 10: "+3V3", 11: "MOT_1B", 12: "MOT_1A", 13: "MOT_2A", 14: "MOT_2B",
           15: "GND", 16: "+12V"},
    "U3": {1: "+12V", 2: "GND", 3: "+5V"},
    "D1": {1: "+12V", 2: "VIN"},
    "C1": {1: "+12V", 2: "GND"},
    "C2": {1: "+12V", 2: "GND"},
    "C3": {1: "+12V", 2: "GND"},
    "C4": {1: "+5V", 2: "GND"},
    "C5": {1: "HALL", 2: "GND"},
    "R1": {1: "TMC_UART", 2: "TMC_TX"},
    "R2": {1: "EN", 2: "+3V3"},
    "R3": {1: "HALL", 2: "HALL_EXT"},
    "JP1": {1: "TMC_UART", 2: "PDN4"},
    "JP2": {1: "TMC_UART", 2: "PDN5"},
    "J1": {1: "VIN", 2: "GND"},
    "J2": {1: "MOT_1B", 2: "MOT_1A", 3: "MOT_2A", 4: "MOT_2B"},
    "J3": {1: "+5V", 2: "HALL_EXT", 3: "GND"},
    "J4": {1: "RST_JMP", 2: "GND"},
}
POWER_NETS = {"+12V", "+5V", "+3V3", "GND", "VIN"}

# ref: (symbol, value, footprint lib, footprint name)
PARTS = {
    "U1": ("ESP32-S3-DevKitC-1", "ESP32-S3-DevKitC-1 N16R8", NAME, "ESP32-S3-DevKitC-1_Socket"),
    "U2": ("TMC2209_StepStick", "TMC2209 StepStick", NAME, "TMC2209_StepStick_Socket"),
    "U3": ("R-78E", "R-78E5.0-1.0 / K7805-1000R3", "Converter_DCDC", "Converter_DCDC_RECOM_R-78E-0.5_THT"),
    "D1": ("D_Schottky", "1N5822", "Diode_THT", "D_DO-201AD_P15.24mm_Horizontal"),
    "C1": ("C_Polarized", "100uF 35V", "Capacitor_THT", "CP_Radial_D8.0mm_P3.50mm"),
    "C2": ("C", "100nF 50V", "Capacitor_SMD", "C_1206_3216Metric_Pad1.33x1.80mm_HandSolder"),
    "C3": ("C_Polarized", "10uF 25V", "Capacitor_THT", "CP_Radial_D5.0mm_P2.00mm"),
    "C4": ("C_Polarized", "22uF 10V", "Capacitor_THT", "CP_Radial_D5.0mm_P2.00mm"),
    "C5": ("C", "1nF", "Capacitor_THT", "C_Disc_D5.0mm_W2.5mm_P5.00mm"),
    "R1": ("R", "1k", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"),
    "R2": ("R", "10k", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"),
    "R3": ("R", "1k", "Resistor_THT", "R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal"),
    "JP1": ("SolderJumper", "PDN pin4", "Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm"),
    "JP2": ("SolderJumper", "PDN pin5", "Jumper", "SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm"),
    "J1": ("Conn_01x02", "12V IN (Weidmuller SL 5.08/2/90)", "Connector_Phoenix_MSTB",
           "PhoenixContact_MSTBA_2,5_2-G-5,08_1x02_P5.08mm_Horizontal"),
    "J2": ("Conn_01x04", "MOTOR (Weidmuller SL 5.08/4/90)", "Connector_Phoenix_MSTB",
           "PhoenixContact_MSTBA_2,5_4-G-5,08_1x04_P5.08mm_Horizontal"),
    "J3": ("Conn_01x03", "HALL (Weidmuller SL 5.08/3/90)", "Connector_Phoenix_MSTB",
           "PhoenixContact_MSTBA_2,5_3-G-5,08_1x03_P5.08mm_Horizontal"),
    "J4": ("Conn_01x02", "RESET jumper", "Connector_PinHeader_2.54mm", "PinHeader_1x02_P2.54mm_Vertical"),
    "H1": ("MountingHole", "M3", "MountingHole", "MountingHole_3.2mm_M3"),
    "H2": ("MountingHole", "M3", "MountingHole", "MountingHole_3.2mm_M3"),
    "H3": ("MountingHole", "M3", "MountingHole", "MountingHole_3.2mm_M3"),
    "H4": ("MountingHole", "M3", "MountingHole", "MountingHole_3.2mm_M3"),
}
SYM_UUID = {r: U() for r in PARTS}

# ============================================================================
#  SYMBOLY
# ============================================================================
F = '(effects (font (size 1.27 1.27)))'
FH = '(effects (font (size 1.27 1.27)) (hide yes))'

def pin(num, name, x, y, ang, length=2.54, typ="passive", hide=False):
    h = " (hide yes)" if hide else ""
    return (f'(pin {typ} line (at {x:g} {y:g} {ang}) (length {length:g}){h} '
            f'(name "{name}" {F}) (number "{num}" {F}))')

def rect(x1, y1, x2, y2, fill="background"):
    return (f'(rectangle (start {x1:g} {y1:g}) (end {x2:g} {y2:g}) '
            f'(stroke (width 0.254) (type default)) (fill (type {fill})))')

def poly(pts, w=0.254, fill="none"):
    p = " ".join(f"(xy {x:g} {y:g})" for x, y in pts)
    return f'(polyline (pts {p}) (stroke (width {w:g}) (type default)) (fill (type {fill})))'

def symdef(name, ref, value, body, pins, extra_props="", power=False, pin_numbers=True,
           pin_names=True, ref_at=(0, 0), val_at=(0, 0)):
    """Vrátí (lib_body) – bez prefixu knihovny v názvu (doplní se dle kontextu)."""
    flags = "(power) " if power else ""
    pn = "" if pin_numbers else "(pin_numbers (hide yes)) "
    pnm = "(pin_names (offset 0.254)) " if pin_names else "(pin_names (offset 0) (hide yes)) "
    s = (f'(symbol "{{N}}" {flags}{pn}{pnm}(exclude_from_sim no) (in_bom {"no" if power else "yes"}) '
         f'(on_board {"no" if power else "yes"})\n'
         f'  (property "Reference" "{ref}" (at {ref_at[0]:g} {ref_at[1]:g} 0) {F if not power else FH})\n'
         f'  (property "Value" "{value}" (at {val_at[0]:g} {val_at[1]:g} 0) {F})\n'
         f'  (property "Footprint" "" (at 0 0 0) {FH})\n'
         f'  (property "Datasheet" "" (at 0 0 0) {FH})\n'
         f'  (property "Description" "" (at 0 0 0) {FH})\n'
         f'{extra_props}'
         f'  (symbol "{name}_0_1" {" ".join(body)})\n'
         f'  (symbol "{name}_1_1" {" ".join(pins)})\n'
         f')')
    return s

SYMS = {}   # name -> (text_template, pin geometry {num: (x, y, outward_dx, outward_dy)})

def add_sym(name, text, pins_geo):
    SYMS[name] = (text, pins_geo)

# --- ESP32-S3-DevKitC-1 ------------------------------------------------------
def mk_esp():
    pins, geo = [], {}
    for i in range(22):
        y = (11 - (i + 1)) * 2.54
        pins.append(pin(i + 1, ESP_J1[i], -12.7, y, 0))
        geo[i + 1] = (-12.7, y, -1, 0)
        pins.append(pin(23 + i, ESP_J3[i], 12.7, y, 180))
        geo[23 + i] = (12.7, y, 1, 0)
    body = [rect(-10.16, 27.94, 10.16, -30.48)]
    add_sym("ESP32-S3-DevKitC-1", symdef("ESP32-S3-DevKitC-1", "U", "ESP32-S3-DevKitC-1", body, pins,
            ref_at=(0, 29.21), val_at=(0, -31.75)), geo)

def mk_tmc():
    pins, geo = [], {}
    for i in range(8):
        y = (4 - (i + 1)) * 2.54
        pins.append(pin(i + 1, TMC_PINS[i], -10.16, y, 0))
        geo[i + 1] = (-10.16, y, -1, 0)
        n = 16 - i   # pravá strana shora: VM(16), GND(15), 2B(14) ...
        pins.append(pin(n, TMC_PINS[n - 1], 10.16, y, 180))
        geo[n] = (10.16, y, 1, 0)
    body = [rect(-7.62, 10.16, 7.62, -12.7)]
    add_sym("TMC2209_StepStick", symdef("TMC2209_StepStick", "U", "TMC2209", body, pins,
            ref_at=(0, 11.43), val_at=(0, -13.97)), geo)

def mk_reg():
    pins = [pin(1, "VIN", -10.16, 0, 0), pin(2, "GND", 0, -7.62, 90), pin(3, "VOUT", 10.16, 0, 180)]
    geo = {1: (-10.16, 0, -1, 0), 2: (0, -7.62, 0, -1), 3: (10.16, 0, 1, 0)}
    body = [rect(-7.62, 5.08, 7.62, -5.08)]
    add_sym("R-78E", symdef("R-78E", "U", "R-78E5.0", body, pins, ref_at=(0, 6.35), val_at=(0, -11.43)), geo)

def two_pin(name, ref, body, numbers=False):
    pins = [pin(1, "~", 0, 5.08, 270), pin(2, "~", 0, -5.08, 90)]
    geo = {1: (0, 5.08, 0, 1), 2: (0, -5.08, 0, -1)}
    add_sym(name, symdef(name, ref, name, body, pins, pin_numbers=numbers, pin_names=False,
                         ref_at=(2.54, 1.27), val_at=(2.54, -1.27)), geo)

def mk_passives():
    two_pin("R", "R", [rect(-1.016, 2.54, 1.016, -2.54, "none")])
    two_pin("C", "C", [poly([(-2.032, 0.762), (2.032, 0.762)], 0.508), poly([(-2.032, -0.762), (2.032, -0.762)], 0.508),
                       poly([(0, 2.54), (0, 0.762)]), poly([(0, -2.54), (0, -0.762)])])
    two_pin("C_Polarized", "C", [rect(-2.286, 1.27, 2.286, 0.508, "none"), rect(-2.286, -0.508, 2.286, -1.27, "outline"),
                                 poly([(-1.778, 2.286), (-0.762, 2.286)]), poly([(-1.27, 2.794), (-1.27, 1.778)]),
                                 poly([(0, 2.54), (0, 1.27)]), poly([(0, -2.54), (0, -1.27)])])
    # dioda svisle: katoda (1) nahoře
    two_pin("D_Schottky", "D", [poly([(-1.27, -1.27), (1.27, -1.27), (0, 1.27), (-1.27, -1.27)], 0.254, "none"),
                                poly([(-1.778, 0.762), (-1.778, 1.27), (1.778, 1.27), (1.778, 1.778)]),
                                poly([(0, 2.54), (0, 1.27)]), poly([(0, -2.54), (0, -1.27)])], numbers=True)
    two_pin("SolderJumper", "JP", [rect(-1.27, 2.54, 1.27, 0.508, "outline"), rect(-1.27, -0.508, 1.27, -2.54, "outline")])

def mk_conn(n):
    name = f"Conn_01x{n:02d}"
    pins, geo = [], {}
    for i in range(n):
        y = -i * 2.54
        pins.append(pin(i + 1, f"Pin_{i + 1}", -5.08, y, 0, 3.81))
        geo[i + 1] = (-5.08, y, -1, 0)
    body = [rect(-1.27, 1.27, 1.27, -(n - 1) * 2.54 - 1.27)]
    add_sym(name, symdef(name, "J", name, body, pins, pin_names=False,
                         ref_at=(0, 2.54), val_at=(0, -(n - 1) * 2.54 - 2.54)), geo)

def mk_hole():
    body = [f'(circle (center 0 0) (radius 1.778) (stroke (width 0.254) (type default)) (fill (type none)))']
    add_sym("MountingHole", symdef("MountingHole", "H", "MountingHole", body, [],
                                   ref_at=(2.54, 0.635), val_at=(2.54, -1.27)), {})

def mk_power(net, down=False):
    if down:   # GND
        body = [poly([(0, 0), (0, -1.27), (1.27, -1.27), (0, -2.54), (-1.27, -1.27), (0, -1.27)])]
        val_at = (0, -3.81)
    else:
        body = [poly([(-0.762, 1.27), (0, 2.54)]), poly([(0, 2.54), (0.762, 1.27)]), poly([(0, 0), (0, 2.54)])]
        val_at = (0, 3.556)
    pins = [pin(1, net, 0, 0, 90 if not down else 270, 0, "power_in", hide=True)]
    add_sym(net, symdef(net, "#PWR", net, body, pins, power=True, pin_names=False, val_at=val_at), {1: (0, 0, 0, 0)})

def mk_flag():
    body = [poly([(0, 0), (0, 1.27), (-1.016, 1.905), (0, 2.54), (1.016, 1.905), (0, 1.27)])]
    pins = [pin(1, "pwr", 0, 0, 90, 0, "power_out")]
    t = symdef("PWR_FLAG", "#FLG", "PWR_FLAG", body, pins, power=True, pin_numbers=False, pin_names=False,
               val_at=(0, 3.81))
    add_sym("PWR_FLAG", t, {1: (0, 0, 0, 0)})

mk_esp(); mk_tmc(); mk_reg(); mk_passives()
for n in (2, 3, 4):
    mk_conn(n)
mk_hole()
for n in ("+12V", "+5V", "+3V3", "VIN"):
    mk_power(n)
mk_power("GND", down=True)
mk_flag()

def sym_text(name, prefix=""):
    return SYMS[name][0].replace("{N}", prefix + name)

with open(f"{PRJ}/{NAME}.kicad_sym", "w", encoding="utf-8") as f:
    f.write('(kicad_symbol_lib (version 20241209) (generator "camslider_gen") (generator_version "9.0")\n')
    for n in SYMS:
        f.write(sym_text(n) + "\n")
    f.write(")\n")

# ============================================================================
#  SCHÉMA
# ============================================================================
SCH_POS = {   # ref: (x, y) v mm, mřížka 2.54
    "U1": (101.6, 101.6), "U2": (182.88, 76.2), "R1": (152.4, 50.8), "R2": (139.7, 81.28),
    "JP1": (160.02, 33.02), "JP2": (172.72, 33.02),
    "J2": (241.3, 73.66), "C1": (182.88, 129.54), "C2": (195.58, 129.54),
    "J1": (30.48, 160.02), "D1": (55.88, 157.48), "U3": (101.6, 160.02),
    "C3": (81.28, 167.64), "C4": (127, 167.64),
    "R3": (241.3, 129.54), "C5": (226.06, 134.62), "J3": (269.24, 134.62),
    "J4": (226.06, 160.02),
    "H1": (35.56, 190.5), "H2": (50.8, 190.5), "H3": (66.04, 190.5), "H4": (81.28, 190.5),
}

sch_items = []
pwr_count = [0]

def wire(x1, y1, x2, y2):
    sch_items.append(f'(wire (pts (xy {x1:g} {y1:g}) (xy {x2:g} {y2:g})) (stroke (width 0) (type default)) (uuid "{U()}"))')

def label(net, x, y, dx, dy):
    ang, just = {(1, 0): (0, "left"), (-1, 0): (180, "right"), (0, -1): (90, "left"), (0, 1): (270, "right")}[(dx, dy)]
    sch_items.append(f'(label "{net}" (at {x:g} {y:g} {ang}) (fields_autoplaced yes) '
                     f'(effects (font (size 1.27 1.27)) (justify {just} bottom)) (uuid "{U()}"))')

def no_connect(x, y):
    sch_items.append(f'(no_connect (at {x:g} {y:g}) (uuid "{U()}"))')

def place_symbol(ref, libname, x, y, value, footprint, pin_nums, uid=None, rot=0, in_bom=True):
    uid = uid or U()
    is_pwr = ref.startswith("#")
    pins = " ".join(f'(pin "{p}" (uuid "{U()}"))' for p in pin_nums)
    hide = " (hide yes)" if is_pwr else ""
    ry = y - 5.08 if not libname.endswith("GND") else y + 5.08
    sch_items.append(
        f'(symbol (lib_id "{NAME}:{libname}") (at {x:g} {y:g} {rot}) (unit 1) (exclude_from_sim no) '
        f'(in_bom {"no" if is_pwr or not in_bom else "yes"}) (on_board {"no" if is_pwr else "yes"}) (dnp no) (uuid "{uid}")\n'
        f'  (property "Reference" "{ref}" (at {x + 3.81:g} {ry if is_pwr else y - 2.54:g} 0) (effects (font (size 1.27 1.27)){hide} (justify left)))\n'
        f'  (property "Value" "{value}" (at {x + 3.81 if not is_pwr else x:g} {y + 2.54 if not is_pwr else (y - 3.81 if not libname.endswith("GND") else y + 3.81):g} 0) (effects (font (size 1.27 1.27)){"" if is_pwr else " (justify left)"}))\n'
        f'  (property "Footprint" "{footprint}" (at {x:g} {y:g} 0) {FH})\n'
        f'  (property "Datasheet" "" (at {x:g} {y:g} 0) {FH})\n'
        f'  (property "Description" "" (at {x:g} {y:g} 0) {FH})\n'
        f'  {pins}\n'
        f'  (instances (project "{NAME}" (path "/{ROOT_UUID}" (reference "{ref}") (unit 1))))\n'
        f')')

def power_symbol(net, x, y):
    pwr_count[0] += 1
    place_symbol(f"#PWR{pwr_count[0]:02d}", net, x, y, net, "", [1])

for ref, (sym, val, fplib, fpname) in PARTS.items():
    sx, sy = SCH_POS[ref]
    geo = SYMS[sym][1]
    place_symbol(ref, sym, sx, sy, val, f"{fplib}:{fpname}", list(geo.keys()), SYM_UUID[ref],
                 in_bom=not (ref.startswith("H") or ref.startswith("JP")))
    for pnum, (px, py, odx, ody) in geo.items():
        ax, ay = sx + px, sy - py          # symbol Y nahoru -> schéma Y dolů
        net = NETS.get(ref, {}).get(pnum)
        if net is None:
            no_connect(ax, ay)
            continue
        ex, ey = ax + odx * 2.54, ay - ody * 2.54
        wire(ax, ay, ex, ey)
        if net in POWER_NETS:
            power_symbol(net, ex, ey)
        else:
            label(net, ex, ey, odx, -ody)

# PWR_FLAG pro napájecí sítě
for i, net in enumerate(["VIN", "+12V", "+5V", "+3V3", "GND"]):
    x, y = 152.4 + i * 15.24, 182.88
    place_symbol(f"#FLG{i + 1:02d}", "PWR_FLAG", x, y, "PWR_FLAG", "", [1])
    wire(x, y, x, y + 2.54)
    power_symbol(net, x, y + 2.54)

def text(t, x, y, size=1.27):
    t = t.replace('"', '\\"').replace("\n", "\\n")
    sch_items.append(f'(text "{t}" (exclude_from_sim no) (at {x:g} {y:g} 0) '
                     f'(effects (font (size {size:g} {size:g})) (justify left bottom)) (uuid "{U()}"))')

text("ESP32-S3-DevKitC-1 (N16R8) – zasunuto do dutinkových lišt", 76.2, 66.04, 1.778)
text("TMC2209 StepStick, adresa 0 (MS1=MS2=GND)\nUART: TX -> 1k -> PDN_UART <- RX\nJP1/JP2: propoj JEN JEDEN podle modulu\n(BTT/FYSETC: obvykle pin 4 = JP1)", 157.48, 20.32)
text("Napájení 12 V: ochrana proti přepólování D1, 5 V z R-78E (1 A)", 25.4, 144.78, 1.778)
text("Hallovo čidlo: open-collector, napájení 5 V,\nvýstup tažen interním pull-upem ESP na 3V3.\nR3 + C5 = ochrana / filtr", 220.98, 116.84)
text("Reset jumper: osazeno = normál, rozpojeno = reset nastavení", 210.82, 152.4)
text("Motor: 1A/1B = cívka A, 2A/2B = cívka B", 226.06, 60.96)

with open(f"{PRJ}/{NAME}.kicad_sch", "w", encoding="utf-8") as f:
    f.write('(kicad_sch (version 20250114) (generator "eeschema") (generator_version "9.0")\n')
    f.write(f'(uuid "{ROOT_UUID}")\n(paper "A4")\n')
    f.write('(title_block (title "CameraSlider – řídicí deska") (rev "1.0") (company ""))\n')
    f.write("(lib_symbols\n")
    used = {PARTS[r][0] for r in PARTS} | POWER_NETS | {"PWR_FLAG"}
    for n in SYMS:
        if n in used:
            f.write(sym_text(n, NAME + ":") + "\n")
    f.write(")\n")
    for it in sch_items:
        f.write(it + "\n")
    f.write(f'(sheet_instances (path "/" (page "1")))\n(embedded_fonts no)\n)\n')

# ============================================================================
#  VLASTNÍ FOOTPRINTY
# ============================================================================
os.makedirs(f"{PRJ}/{NAME}.pretty", exist_ok=True)

def fp_pad(num, x, y, shape="circle", size=1.7, drill=1.0):
    return (f'  (pad "{num}" thru_hole {shape} (at {x:g} {y:g}) (size {size:g} {size:g}) (drill {drill:g}) '
            f'(layers "*.Cu" "*.Mask") (remove_unused_layers no))\n')

def fp_line(x1, y1, x2, y2, layer, w=0.12):
    return f'  (fp_line (start {x1:g} {y1:g}) (end {x2:g} {y2:g}) (stroke (width {w:g}) (type solid)) (layer "{layer}"))\n'

def fp_rect(x1, y1, x2, y2, layer, w=0.12):
    return f'  (fp_rect (start {x1:g} {y1:g}) (end {x2:g} {y2:g}) (stroke (width {w:g}) (type solid)) (fill no) (layer "{layer}"))\n'

def fp_text(t, x, y, layer="F.SilkS", size=1.0, rot=0):
    return (f'  (fp_text user "{t}" (at {x:g} {y:g} {rot}) (layer "{layer}") '
            f'(effects (font (size {size:g} {size:g}) (thickness {size * 0.15:g}))))\n')

def fp_head(name, descr, ref_at, val_at):
    return (f'(footprint "{name}" (version 20241229) (generator "camslider_gen") (layer "F.Cu")\n'
            f'  (descr "{descr}")\n'
            f'  (property "Reference" "REF**" (at {ref_at[0]:g} {ref_at[1]:g} 0) (layer "F.SilkS") (effects (font (size 1 1) (thickness 0.15))))\n'
            f'  (property "Value" "{name}" (at {val_at[0]:g} {val_at[1]:g} 0) (layer "F.Fab") (effects (font (size 1 1) (thickness 0.15))))\n'
            f'  (attr through_hole)\n')

P = 2.54
# --- ESP32-S3-DevKitC-1: J1 na y=0 (pin1 vlevo), J3 dvojitá řada: 25.4 mm i 22.86 mm ----
s = fp_head("ESP32-S3-DevKitC-1_Socket",
            "ESP32-S3-DevKitC-1 v dutinkových lištách 2x22; J3 ve dvou variantách rozteče řad (25.4 / 22.86 mm), "
            "osaď lištu jen do řady, která sedí na tvou desku", (26.67, -12.7), (26.67, -15.5))
for i in range(22):
    s += fp_pad(i + 1, i * P, 0, "rect" if i == 0 else "circle")
    s += fp_pad(23 + i, i * P, -25.4, "rect" if i == 0 else "circle")
    s += fp_pad(23 + i, i * P, -22.86, "rect" if i == 0 else "circle")
# silk: lišty
s += fp_rect(-1.4, -1.4, 21 * P + 1.4, 1.4, "F.SilkS")
s += fp_rect(-1.4, -26.8, 21 * P + 1.4, -21.46, "F.SilkS")
s += fp_line(-1.4, -24.13, 21 * P + 1.4, -24.13, "F.SilkS", 0.1)
s += fp_text("J1", -3.2, 0, size=1.0)
s += fp_text("25.4", -3.6, -25.4, size=0.8)
s += fp_text("22.86", -3.9, -22.86, size=0.8)
s += fp_text("ESP32-S3-DevKitC-1", 26.67, -6.5, size=1.5)
s += fp_text("USB  >>>", 52, -12.7, size=1.2)
s += fp_text("3V3", 1.27, 2.4, size=0.8)
s += fp_text("5V", 20 * P, 2.4, size=0.8)
s += fp_text("GND", 21 * P, 2.4, size=0.8)
# fab: přibližný obrys devkitu (62.7 x 25.4) – ověř na reálné desce
s += fp_rect(-4.7, -26.7, 58.0, 1.3, "F.Fab", 0.1)
s += fp_rect(-1.65, -27.05, 21 * P + 1.65, 1.65, "F.CrtYd", 0.05)
s += ")\n"
open(f"{PRJ}/{NAME}.pretty/ESP32-S3-DevKitC-1_Socket.kicad_mod", "w", encoding="utf-8").write(s)

# --- TMC2209 StepStick: horní řada logika (DIR..EN), dolní výkon (GND..VM) -------
s = fp_head("TMC2209_StepStick_Socket",
            "StepStick (TMC2209) v dutinkových lištách 2x8, rozteč řad 12.7 mm, otočeno o 90 st.: "
            "logika nahoře, EN vpravo nahoře, VM vpravo dole", (8.89, 8.0), (8.89, 16.5))
for k in range(1, 9):
    s += fp_pad(k, (8 - k) * P, 0, "rect" if k == 1 else "circle")
for k in range(9, 17):
    s += fp_pad(k, (k - 9) * P, 12.7, "circle")
s += fp_rect(-1.4, -1.4, 7 * P + 1.4, 1.4, "F.SilkS")
s += fp_rect(-1.4, 11.3, 7 * P + 1.4, 14.1, "F.SilkS")
for k in range(1, 9):
    s += fp_text(TMC_PINS[k - 1], (8 - k) * P, 3.6, size=0.8, rot=90)
for k in range(9, 17):
    s += fp_text(TMC_PINS[k - 1], (k - 9) * P, 9.1, size=0.8, rot=90)
s += fp_text("TMC2209", 8.89, 6.1, size=1.0)
s += fp_rect(-1.27, -1.27, 7 * P + 1.27, 13.97, "F.Fab", 0.1)
s += fp_rect(-1.65, -1.65, 7 * P + 1.65, 14.35, "F.CrtYd", 0.05)
s += ")\n"
open(f"{PRJ}/{NAME}.pretty/TMC2209_StepStick_Socket.kicad_mod", "w", encoding="utf-8").write(s)

# lib tabulky
open(f"{PRJ}/fp-lib-table", "w").write(
    f'(fp_lib_table (version 7)\n  (lib (name "{NAME}") (type "KiCad") (uri "${{KIPRJMOD}}/{NAME}.pretty") (options "") (descr "CameraSlider"))\n)\n')
open(f"{PRJ}/sym-lib-table", "w").write(
    f'(sym_lib_table (version 7)\n  (lib (name "{NAME}") (type "KiCad") (uri "${{KIPRJMOD}}/{NAME}.kicad_sym") (options "") (descr "CameraSlider"))\n)\n')

# ============================================================================
#  PCB
# ============================================================================
OX, OY = 50.0, 40.0        # posun desky na listu
W, H = 110.0, 86.0
mm = pcbnew.FromMM

def V(x, y):
    return pcbnew.VECTOR2I(mm(x + OX), mm(y + OY))

print("pcb start",flush=True)
board = pcbnew.BOARD()
print("board",flush=True)
ds = board.GetDesignSettings()
ds.SetCopperLayerCount(2)
nc = ds.m_NetSettings.GetDefaultNetclass()
nc.SetClearance(mm(0.25)); nc.SetTrackWidth(mm(0.4)); nc.SetViaDiameter(mm(0.8)); nc.SetViaDrill(mm(0.4))

print("ds ok",flush=True)
netobj = {}
def net(n):
    if n not in netobj:
        ni = pcbnew.NETINFO_ITEM(board, n)
        board.Add(ni)
        netobj[n] = ni
    return netobj[n]

fps = {}
PINNAMES = {"U1": {i + 1: (ESP_J1 + ESP_J3)[i] for i in range(44)},
            "U2": {i + 1: TMC_PINS[i] for i in range(16)}}
# ref: (rot, anchor_pad nebo "c", x, y)
PLACE = {
    "U1": (0, "1", 22.0, 32.0),
    "U2": (0, "8", 27.08, 46.0),
    "U3": (0, "1", 70.0, 60.0),
    "D1": (0, "1", 74.0, 68.0),
    "C1": (90, "1", 56.0, 64.0),
    "C2": (270, "1", 49.3, 60.94),
    "C3": (0, "1", 66.0, 72.0),
    "C4": (0, "1", 82.0, 58.0),
    "C5": (0, "1", 85.0, 38.5),
    "R1": (0, "1", 37.24, 38.0),
    "R2": (270, "1", 49.5, 49.0),
    "R3": (0, "1", 85.0, 35.0),
    "JP1": (270, "c", 37.54, 42.4),
    "JP2": (270, "c", 34.40, 42.4),
    "J1": (0, "1", 92.0, 75.0),
    "J2": (0, "1", 29.0, 75.0),
    "J3": (90, "1", 98.0, 50.0),
    "J4": (0, "1", 62.64, 38.0),
    "H1": (0, "c", 4.0, 4.0), "H2": (0, "c", W - 4, 4.0),
    "H3": (0, "c", 4.0, H - 4), "H4": (0, "c", W - 4, H - 4),
}
for ref, (sym, val, lib, fpn) in PARTS.items():
    libpath = f"{PRJ}/{NAME}.pretty" if lib == NAME else STOCK + lib + ".pretty"
    fp = pcbnew.FootprintLoad(libpath, fpn)
    assert fp is not None, fpn
    fp.SetFPID(pcbnew.LIB_ID(lib, fpn))
    fp.SetReference(ref)
    fp.SetValue(val)
    rot, anc, x, y = PLACE[ref]
    fp.SetPosition(pcbnew.VECTOR2I(0, 0))
    fp.SetOrientationDegrees(rot)
    if anc == "c":
        off = pcbnew.VECTOR2I(0, 0)
    else:
        off = [p for p in fp.Pads() if p.GetNumber() == anc][0].GetPosition()
    fp.SetPosition(V(x, y) - off)
    fp.SetPath(pcbnew.KIID_PATH("/" + SYM_UUID[ref]))
    fp.SetSheetname("Root")
    fp.SetSheetfile(f"{NAME}.kicad_sch")
    for p in fp.Pads():
        if p.GetNumber() == "":
            continue
        num = int(p.GetNumber())
        n = NETS.get(ref, {}).get(num)
        if n:
            p.SetNet(net(n if n in POWER_NETS else "/" + n))
        elif ref in PINNAMES:
            p.SetNet(net(f"unconnected-({ref}-{PINNAMES[ref][num]}-Pad{num})"))
    if ref.startswith("H") or ref.startswith("JP"):
        fp.Reference().SetVisible(False)
    board.Add(fp)
    fps[ref] = fp

# hodnoty/reference: drobné úpravy polohy textů
def padpos(ref, num):
    for p in fps[ref].Pads():
        if p.GetNumber() == str(num):
            v = p.GetPosition()
            return (pcbnew.ToMM(v.x) - OX, pcbnew.ToMM(v.y) - OY)
    raise KeyError((ref, num))

WIDTH = {"+12V": 1.5, "VIN": 1.5, "MOT_1A": 1.0, "MOT_1B": 1.0, "MOT_2A": 1.0, "MOT_2B": 1.0,
         "+5V": 0.8, "+3V3": 0.5, "GND": 0.5}

def track(netname, layer, pts, width=None):
    w = width or WIDTH.get(netname, 0.4)
    lay = pcbnew.F_Cu if layer == "F" else pcbnew.B_Cu
    pts = [padpos(*p) if isinstance(p[0], str) else p for p in pts]
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        t = pcbnew.PCB_TRACK(board)
        t.SetStart(V(x1, y1)); t.SetEnd(V(x2, y2))
        t.SetWidth(mm(w)); t.SetLayer(lay); t.SetNet(net(netname if netname in POWER_NETS else "/" + netname))
        board.Add(t)

print("fps ok",flush=True)
def setref(ref, x, y):
    fps[ref].Reference().SetPosition(V(x, y))
setref("C5", 87.5, 41.4); setref("C2", 51.9, 66.0); setref("U2", 24.0, 52.4); setref("C1", 62.8, 62.0)
# zdvojené řady J3 (25.4 / 22.86 mm) – nezapojené piny propojit krátkou spojkou
for num in range(24, 43):
    p = [q for q in fps["U1"].Pads() if q.GetNumber() == str(num)]
    a, b = p[0].GetPosition(), p[1].GetPosition()
    t = pcbnew.PCB_TRACK(board); t.SetStart(a); t.SetEnd(b); t.SetWidth(mm(0.4))
    t.SetLayer(pcbnew.B_Cu); t.SetNet(p[0].GetNet()); board.Add(t)
# ---- trasy -------------------------------------------------------------------
track("STEP", "F", [("U1", 4), ("U2", 7)])
track("DIR", "B", [("U1", 5), (32.16, 36.0), (27.08, 36.0), ("U2", 8)])
track("EN", "B", [("U1", 6), (34.70, 34.4), (49.5, 34.4), ("R2", 1)])
track("EN", "B", [(49.5, 46.0), ("U2", 1)])
track("TMC_TX", "F", [("U1", 10), ("R1", 2)])
track("TMC_UART", "F", [("U1", 11), (47.40, 41.0), (37.24, 41.0)])
track("TMC_UART", "F", [("R1", 1), (37.24, 41.0), (37.54, 41.3), ("JP1", 1)])
track("TMC_UART", "F", [(37.24, 41.0), (34.40, 41.0), ("JP2", 1)])
track("PDN5", "F", [("JP2", 2), ("U2", 5)])
track("PDN4", "F", [("JP1", 2), ("U2", 4)])
track("GND", "F", [("U2", 3), ("U2", 2), ("U2", 15)])
track("+3V3", "F", [("U1", 1), ("U1", 2)])
track("+3V3", "F", [("U1", 1), (22.0, 61.5), (29.62, 61.5), ("U2", 10)])
track("+3V3", "B", [("U2", 10), (29.62, 64.5), (49.5, 64.5), ("R2", 2)])
track("MOT_1B", "F", [("U2", 11), (32.16, 66.0), (29.0, 69.16), ("J2", 1)])
track("MOT_1A", "F", [("U2", 12), (34.70, 69.0), (34.08, 69.62), ("J2", 2)])
track("MOT_2A", "F", [("U2", 13), (37.24, 69.0), (39.16, 70.92), ("J2", 3)])
track("MOT_2B", "F", [("U2", 14), (39.78, 64.0), (44.24, 68.46), ("J2", 4)])
track("+12V", "F", [("U2", 16), (47.0, 60.84), (47.0, 66.5), (48.5, 68.0), ("D1", 1)])
track("+12V", "F", [(47.0, 60.84), ("C2", 1)], 1.0)
track("+12V", "F", [("C1", 1), (56.0, 68.0)])
track("+12V", "F", [("U3", 1), (70.0, 68.0)])
track("+12V", "F", [("C3", 1), (66.0, 68.0)], 1.0)
track("VIN", "F", [("D1", 2), (92.0, 70.76), ("J1", 1)])
track("+5V", "F", [("U3", 3), (80.0, 60.0), ("C4", 1)])
track("+5V", "F", [("C4", 1), (82.0, 36.5), (72.80, 36.5), ("U1", 21)])
track("+5V", "F", [(82.0, 50.0), ("J3", 1)])
track("RST_JMP", "F", [("U1", 17), ("J4", 1)])
track("HALL", "B", [("U1", 16), (60.10, 35.0), ("R3", 1)])
track("HALL", "B", [("R3", 1), ("C5", 1)])
track("HALL_EXT", "B", [("R3", 2), (95.0, 37.38), (95.0, 44.92), ("J3", 2)])

# ---- GND prošívací průchodky -------------------------------------------------
for (x, y) in [(10, 20), (10, 45), (10, 65), (18, 78), (30, 18), (50, 18), (70, 18), (90, 18),
               (55, 40), (68, 45), (90, 30), (104, 62), (56, 80), (76, 79), (40, 38), (20, 52)]:
    v = pcbnew.PCB_VIA(board)
    v.SetPosition(V(x, y)); v.SetWidth(mm(0.8)); v.SetDrill(mm(0.4))
    v.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu); v.SetNet(net("GND"))
    board.Add(v)

# ---- obrys desky (zaoblené rohy r=3) ----------------------------------------
R = 3.0
def edge_line(x1, y1, x2, y2):
    s = pcbnew.PCB_SHAPE(board); s.SetShape(pcbnew.SHAPE_T_SEGMENT)
    s.SetStart(V(x1, y1)); s.SetEnd(V(x2, y2)); s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.1))
    board.Add(s)
def edge_arc(cx, cy, sx, sy, ang):
    s = pcbnew.PCB_SHAPE(board); s.SetShape(pcbnew.SHAPE_T_ARC)
    s.SetCenter(V(cx, cy)); s.SetStart(V(sx, sy)); s.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(ang, pcbnew.DEGREES_T))
    s.SetLayer(pcbnew.Edge_Cuts); s.SetWidth(mm(0.1))
    board.Add(s)
edge_line(R, 0, W - R, 0); edge_line(W, R, W, H - R); edge_line(W - R, H, R, H); edge_line(0, H - R, 0, R)
edge_arc(R, R, 0, R, 90)
edge_arc(W - R, R, W - R, 0, 90)
edge_arc(W - R, H - R, W, H - R, 90)
edge_arc(R, H - R, R, H, 90)

# ---- GND zóny na obou vrstvách ----------------------------------------------
for lay in (pcbnew.F_Cu, pcbnew.B_Cu):
    z = pcbnew.ZONE(board)
    z.SetLayer(lay); z.SetNet(net("GND"))
    o = z.Outline(); o.NewOutline()
    for (x, y) in [(0.3, 0.3), (W - 0.3, 0.3), (W - 0.3, H - 0.3), (0.3, H - 0.3)]:
        o.Append(mm(x + OX), mm(y + OY))
    z.SetLocalClearance(mm(0.4))
    z.SetMinThickness(mm(0.25))
    z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
    z.SetThermalReliefGap(mm(0.4))
    z.SetThermalReliefSpokeWidth(mm(0.6))
    z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    z.SetZoneName("GND")
    board.Add(z)

# ---- popisky -----------------------------------------------------------------
def silk(t, x, y, size=1.0, rot=0, layer=pcbnew.F_SilkS):
    tx = pcbnew.PCB_TEXT(board)
    tx.SetText(t); tx.SetPosition(V(x, y)); tx.SetLayer(layer)
    tx.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size))); tx.SetTextThickness(mm(size * 0.15))
    tx.SetTextAngleDegrees(rot)
    board.Add(tx)

silk("+12V", 92.0, 70.4, 1.0); silk("GND", 97.08, 70.4, 1.0)
silk("1B", 27.0, 70.6, 0.9); silk("1A", 32.3, 71.3, 0.9); silk("2A", 41.2, 71.3, 0.9); silk("2B", 46.5, 71.3, 0.9)
silk("MOTOR", 16.0, 75.0, 1.2)
silk("12V IN", 82.0, 74.5, 1.2)
silk("+5V", 92.5, 51.6, 0.9); silk("SIG", 92.5, 44.9, 0.9); silk("GND", 92.8, 41.0, 0.9)
silk("HALL", 104.0, 56.0, 1.2, 90)
silk("RESET", 66.6, 39.3, 0.9)
silk("JP2", 32.4, 42.4, 0.8, 90); silk("JP1", 39.5, 42.4, 0.8, 90)
silk("PDN: propoj JP1 NEBO JP2", 14.0, 41.0, 0.8)
silk("CameraSlider v1.0", 12.0, 72.0, 1.5)


pcbnew.SaveBoard(f"{PRJ}/{NAME}.kicad_pcb", board)
print("saved",flush=True)
print("OK")
for ref in ("C1", "C2", "R2", "JP1", "JP2", "J3", "D1"):
    print(ref, [(p.GetNumber(), padpos(ref, p.GetNumber())) for p in fps[ref].Pads()])
