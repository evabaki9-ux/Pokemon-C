"""Compile + validate the world maps into src/data/maps.c.

Maps are authored as ASCII grids; this script checks rectangularity, legend
membership, warp targets, NPC/sign placement, and emits C data.
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

LEGEND = set('.,\"TWPFRLS^()#ODH_IBtKMNCprgs=')
WALKABLE = set('.,\"P=LDH_Mrgs')
SOLID = set('TWFRS^()#OIBtKCNp')

# ---------------------------------------------------------------- maps
HOUSE = [
    "IIIIIIIIIII",
    "I_________I",
    "I_B__t____I",
    "I____r____I",
    "I____r____I",
    "I_________I",
    "I_K___C__pI",
    "I_________I",
    "IIIIIMIIIII",
]
LAB = [
    "IIIIIIIIIIIII",
    "I___K___K___I",
    "I___________I",
    "I__C_____C__I",
    "I___________I",
    "I_____rr____I",
    "I_____rr____I",
    "I___________I",
    "I___________I",
    "IIIIIIMIIIIII",
]
HEAL = [
    "IIIIIIIIII",
    "I________I",
    "I________I",
    "INNNNN__pI",
    "I________I",
    "I_r______I",
    "I________I",
    "IIIMIIIIII",
]
MART = [
    "IIIIIIIIII",
    "I________I",
    "I__K__K__I",
    "INNNNN__pI",
    "I________I",
    "I_r______I",
    "I________I",
    "IIIMIIIIII",
]
TOWN = [
    "T" * 13 + "gg" + "T" * 13,                            # 0  north gap -> ROUTE 1
    "T" + "g" * 26 + "T",                                  # 1
    "T" + "g" * 26 + "T",                                  # 2
    "T" + "gg,g,,,," + "g" * 8 + ",,,,," + "ggggg" + "T",  # 3
    "T" + "g" * 16 + "(^^^^)" + "g" * 4 + "T",             # 4  care station roof
    "T" + "gg" + "(^^^)" + "g" * 9 + "#OHOO#" + "g" * 4 + "T",  # 5  house roof / care wall
    "T" + "gg" + "#ODO#" + "g" * 19 + "T",                 # 6  house wall (door x5)
    "T" + "g" * 4 + "P" + "g" * 13 + "P" + "g" + "(^^^^)" + "T",  # 7  mart roof (x21-26)
    "T" + "g" * 4 + "P" + "g" * 13 + "P" + "g" + "#ODOO#" + "T",  # 8  mart wall, door x23
    "T" + "g" * 4 + "P" * 19 + "g" * 3 + "T",                     # 9  main path reaches mart door
    "T" + "g" * 12 + "PP" + "g" * 12 + "T",                # 10
    "T" + "g" * 12 + "PP" + "g" * 12 + "T",                # 11
    "T" + "g" * 12 + "PP" + "g" * 12 + "T",                # 12
    "T" + "g" * 9 + "(^^^^^)" + "g" * 10 + "T",            # 13 lab roof
    "T" + "g" * 9 + "#OODOO#" + "g" * 10 + "T",            # 14 lab wall (door x13)
    "T" + "g" * 12 + "P" + "g" * 13 + "T",                 # 15
    "T" + "g" * 12 + "PgS" + "g" * 11 + "T",               # 16 town sign
    "T" + "g" + "s" + "W" * 7 + "s" + "g" * 16 + "T",      # 17 pond
    "T" + "g" + "s" + "W" * 7 + "s" + "g" * 16 + "T",      # 18 pond
    "T" + "g" + "s" * 9 + "g" * 16 + "T",                  # 19
    "T" + "g" * 11 + "F" * 7 + "g" * 8 + "T",              # 20 garden fence
    "T" + "g" * 11 + "F" + "," * 5 + "F" + "g" * 8 + "T",  # 21
    "T" + "g" * 11 + "F" * 7 + "g" * 8 + "T",              # 22
    "T" * 28,                                               # 23
]
ROUTE1 = [
    "T" * 20,                                                    # 0
    "T" + "g" * 9 + "RRR" + "g" * 6 + "T",                       # 1  landslide
    "T" + "g" * 9 + "RRR" + "g" * 6 + "T",                       # 2
    "T" + "g" * 3 + ",," + "g" * 4 + ",," + "g" * 7 + "T",       # 3
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 4
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 5
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 6
    "T" + "g" * 4 + '"' * 2 + "g" * 3 + "PP" + "g" * 3 + '"' * 2 + "g" * 2 + "T",  # 7
    "T" + "g" * 4 + '"' * 2 + "g" * 3 + "PP" + "g" * 3 + '"' * 2 + "g" * 2 + "T",  # 8
    "T" + "g" * 4 + '"' * 2 + "g" * 3 + "PP" + "g" * 3 + '"' * 2 + "g" * 2 + "T",  # 9
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 10
    "T" + "g" * 2 + "R" + "g" * 6 + "PP" + "g" * 6 + "R" + "T",  # 11
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 12
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 13
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 14  trainer at x11
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 15
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 16
    "T" + "g" * 9 + "PP" + "L" * 6 + "g" + "T",                  # 17 ledge row (hop south)
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 18
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 19
    "T" + "g" * 4 + "R" + "g" * 4 + "PP" + "g" * 7 + "T",        # 20
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 21
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 2 + '"' * 3 + "g" * 2 + "T",  # 22
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 2 + '"' * 3 + "g" * 2 + "T",  # 23
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 2 + '"' * 3 + "g" * 2 + "T",  # 24
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 25
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 26
    "T" + "g" * 9 + "PP" + "g" * 4 + "S" + "g" * 2 + "T",        # 27 sign at x16
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 28
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 29
    "T" + "g" * 2 + '"' * 4 + "g" * 3 + "PP" + "g" * 7 + "T",    # 30
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 31
    "T" + "g" * 9 + "PP" + "g" * 7 + "T",                        # 32
    "T" * 10 + ".." + "T" * 8,                                   # 33 south gap -> town
]

MAPS_ASCII = {
    0: ("PLAYER'S HOUSE", HOUSE),
    1: ("MAPLE'S LAB", LAB),
    2: ("CARE STATION", HEAL),
    3: ("VERDAN TOWN", TOWN),
    4: ("ROUTE 1", ROUTE1),
    5: ("VERDAN MART", MART),
}

# ---------------------------------------------------------------- data
# sprite ids: 0 player, 1 prof/adult, 2 mom, 3 nurse, 4 kid
# roles: 0 none, 1 mom, 2 nurse, 3 prof, 4 trainer, 5 blocker
dlg_kid_pond = [
    "There's a huge fish in here!",
    "It splashed me! Soaked!",
    None,
]
dlg_assistant = [
    "PROF. MAPLE studies the wild",
    "creatures of our region.",
    "She's waiting for you - go",
    "talk to her!",
    None,
]
dlg_hiker = [
    "Whoa there! A landslide has",
    "closed the road north.",
    "Come back once it's cleared!",
    None,
]
dlg_villager = [
    "Tall grass hides creatures!",
    "Walk through it and you're",
    "bound to meet one!",
    None,
]
dlg_liam_pre = [
    "Hey! You have creatures too?",
    "Let's see whose partner is",
    "stronger!",
    None,
]

TRAINER_LIAM = {
    "name": "CAMPER LIAM", "sprite": 4,
    "party": [3, 5],  # FLUFFIT, ZEPHIRD
    "levels": [4, 4], "reward": 320,
}

# map id -> warps (x, y, dest_map, dest_x, dest_y)
WARPS = {
    3: [  # TOWN
        (13, 0, 4, 10, 32), (14, 0, 4, 11, 32),
        (5, 6, 0, 5, 8),    # player house door
        (19, 5, 2, 3, 7),   # care station door
        (13, 14, 1, 6, 9),  # lab door
        (23, 8, 5, 3, 7),   # mart door
    ],
    0: [(5, 8, 3, 5, 7)],    # house mat -> town below door
    1: [(6, 9, 3, 13, 15)],  # lab mat -> town below lab door
    2: [(3, 7, 3, 19, 6)],   # heal mat -> town below heal door
    4: [(10, 33, 3, 13, 1), (11, 33, 3, 14, 1)],
    5: [(3, 7, 3, 23, 9)],   # mart mat -> town below mart door
}
NPCS = {
    0: [  # house
        {"x": 5, "y": 4, "dir": 0, "sprite": 2, "role": 1, "lines": None},
    ],
    1: [  # lab
        {"x": 6, "y": 5, "dir": 0, "sprite": 1, "role": 3, "lines": None},
        {"x": 3, "y": 6, "dir": 3, "sprite": 4, "role": 0, "lines": dlg_assistant},
    ],
    2: [  # heal
        {"x": 2, "y": 2, "dir": 0, "sprite": 3, "role": 2, "lines": None},
    ],
    3: [  # town
        {"x": 6, "y": 16, "dir": 1, "sprite": 4, "role": 0, "lines": dlg_kid_pond},
        {"x": 8, "y": 20, "dir": 2, "sprite": 1, "role": 0, "lines": dlg_villager},
    ],
    4: [  # route 1
        {"x": 11, "y": 14, "dir": 2, "sprite": 4, "role": 4, "lines": dlg_liam_pre,
         "trainer": TRAINER_LIAM},
        {"x": 10, "y": 3, "dir": 0, "sprite": 1, "role": 5, "lines": dlg_hiker},
    ],
    5: [  # mart
        {"x": 2, "y": 2, "dir": 0, "sprite": 1, "role": 6, "lines": None},
    ],
}
SIGNS = {
    3: [(15, 16, "VERDAN TOWN\nWhere journeys sprout.")],
    4: [(16, 27, "ROUTE 1\nVERDAN TOWN - MT. CINDER")],
}
ENCS = {
    4: [
        (3, 2, 4, 45),   # FLUFFIT
        (5, 2, 4, 25),   # ZEPHIRD
        (6, 2, 3, 15),   # MOSSLING
        (7, 3, 4, 10),   # SPARKIT
        (4, 3, 5, 5),    # PEBBLY
    ],
}


def validate():
    errors = []
    for mid, (name, rows) in MAPS_ASCII.items():
        w = len(rows[0])
        for y, row in enumerate(rows):
            if len(row) != w:
                errors.append(f"{name}: row {y} width {len(row)} != {w}")
            for x, ch in enumerate(row):
                if ch not in LEGEND:
                    errors.append(f"{name}: bad char '{ch}' at {x},{y}")
        for (x, y, dm, dx, dy) in WARPS.get(mid, []):
            if not (0 <= y < len(rows) and 0 <= x < len(rows[y])):
                errors.append(f"{name}: warp source out of range {x},{y}")
                continue
            if rows[y][x] not in WALKABLE:
                errors.append(f"{name}: warp source on solid tile at {x},{y} ('{rows[y][x]}')")
            drows = MAPS_ASCII[dm][1]
            if not (0 <= dy < len(drows) and 0 <= dx < len(drows[dy])):
                errors.append(f"{name}: warp dest out of range map{dm} {dx},{dy}")
            elif drows[dy][dx] not in WALKABLE:
                errors.append(f"{name}: warp dest on solid tile map{dm} {dx},{dy} ('{drows[dy][dx]}')")
        for npc in NPCS.get(mid, []):
            ch = rows[npc["y"]][npc["x"]]
            if ch not in WALKABLE:
                errors.append(f"{name}: npc on solid tile at {npc['x']},{npc['y']} ('{ch}')")
        for (x, y, _t) in SIGNS.get(mid, []):
            if rows[y][x] != "S":
                errors.append(f"{name}: sign not on 'S' tile at {x},{y} ('{rows[y][x]}')")
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch in "DMH":
                    if not any(wx == x and wy == y for (wx, wy, *_r) in WARPS.get(mid, [])):
                        errors.append(f"{name}: door '{ch}' at {x},{y} has no warp")
    return errors


def c_str(s):
    out = s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return f'"{out}"'


def emit():
    parts = []
    parts.append('/* Generated by tools/gen_maps.py - do not edit by hand. */')
    parts.append('#include "game.h"')
    parts.append("")
    for mid, (name, rows) in MAPS_ASCII.items():
        ident = name.lower().replace(" ", "_").replace("'", "").replace(".", "")
        parts.append(f"static const char *const rows_{ident}[] = {{")
        for row in rows:
            parts.append(f"    {c_str(row)},")
        parts.append("};")
        parts.append("")
    for mid in sorted(WARPS):
        parts.append(f"static const Warp warps_{mid}[] = {{")
        for (x, y, dm, dx, dy) in WARPS[mid]:
            parts.append(f"    {{{x}, {y}, {dm}, {dx}, {dy}}},")
        parts.append("};")
        parts.append("")
    dlg_map = {
        "dlg_kid_pond": dlg_kid_pond, "dlg_assistant": dlg_assistant,
        "dlg_hiker": dlg_hiker, "dlg_villager": dlg_villager,
        "dlg_liam_pre": dlg_liam_pre,
    }
    for ident, lines in dlg_map.items():
        parts.append(f"static const char *const {ident}[] = {{")
        for l in lines:
            if l is not None:
                parts.append(f"    {c_str(l)},")
        parts.append("    NULL};")
    parts.append("")
    t = TRAINER_LIAM
    parts.append(
        f"static const TrainerDef trainer_liam = {{ \"{t['name']}\", {t['sprite']}, "
        f"{{ {', '.join(map(str, t['party']))} }}, {{ {', '.join(map(str, t['levels']))} }}, "
        f"{len(t['party'])}, {t['reward']} }};")
    parts.append("")
    for mid in sorted(NPCS):
        parts.append(f"static const NpcDef npcs_{mid}[] = {{")
        for npc in NPCS[mid]:
            lines = "NULL" if npc["lines"] is None else \
                [k for k, v in dlg_map.items() if v is npc["lines"]][0]
            tr = "NULL" if "trainer" not in npc else "&trainer_liam"
            parts.append(
                f"    {{{npc['x']}, {npc['y']}, {npc['dir']}, {npc['sprite']}, "
                f"{npc['role']}, {lines}, {tr}}},")
        parts.append("};")
        parts.append("")
    for mid in sorted(SIGNS):
        if not SIGNS[mid]:
            continue
        parts.append(f"static const SignDef signs_{mid}[] = {{")
        for (x, y, text) in SIGNS[mid]:
            parts.append(f"    {{{x}, {y}, {c_str(text)}}},")
        parts.append("};")
        parts.append("")
    for mid in sorted(ENCS):
        if not ENCS[mid]:
            continue
        parts.append(f"static const Encounter encs_{mid}[] = {{")
        for (sp, lo, hi, wt) in ENCS[mid]:
            parts.append(f"    {{{sp}, {lo}, {hi}, {wt}}},")
        parts.append("};")
        parts.append("")
    parts.append("const MapDef MAPS[NUM_MAPS] = {")
    for mid, (name, rows) in sorted(MAPS_ASCII.items()):
        ident = name.lower().replace(" ", "_").replace("'", "").replace(".", "")
        w, h = len(rows[0]), len(rows)
        nw = len(WARPS.get(mid, []))
        nn = len(NPCS.get(mid, []))
        ns = len(SIGNS.get(mid, []))
        ne = len(ENCS.get(mid, []))
        rate = 12 if ENCS.get(mid) else 0
        parts.append(
            f'    {{ "{name}", {w}, {h}, rows_{ident}, '
            f"{'warps_%d' % mid if nw else 'NULL'}, {nw}, "
            f"{'npcs_%d' % mid if nn else 'NULL'}, {nn}, "
            f"{'signs_%d' % mid if ns else 'NULL'}, {ns}, "
            f"{'encs_%d' % mid if ne else 'NULL'}, {ne}, {rate} }},")
    parts.append("};")
    return "\n".join(parts) + "\n"


if __name__ == "__main__":
    errs = validate()
    if errs:
        print("MAP ERRORS:")
        for e in errs:
            print("  -", e)
        raise SystemExit(1)
    out = os.path.join(ROOT, "src", "data", "maps.c")
    with open(out, "w") as f:
        f.write(emit())
    n = sum(len(rows) for _n, rows in MAPS_ASCII.values())
    print(f"maps.c written OK ({len(MAPS_ASCII)} maps, {n} rows)")
