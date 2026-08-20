#!/usr/bin/env python3
"""
Build a 3-page DIPI applicant PDF template.

Why: the 2-page template's free-text boxes are far too small. The renderer
(/dhamma/scripts/dipi-automation/app.py) uses PyMuPDF insert_htmlbox, which
shrinks text to fit rather than clipping it -- so long answers render at
3.6-4.3pt and are unreadable. Bigger boxes keep the scale near 1.0.

Constraints honoured:
  * every field name from the 2-page template is preserved exactly
    (61 text + 42 checkbox), because pdf.inc maps by name
  * the passport photo box stays at its exact original position on page 1,
    because /dhamma/scripts/merge-img stamps the photo there with cpdf at a
    fixed absolute offset ("7.4cm 5.5cm", page 1)
"""
import fitz, json, sys, os

W, H = 612.0, 792.0
L, R = 44.0, 568.0                      # left / right text margin
SERIF, BOLD = "tiro", "tibo"            # Times-Roman, Times-Bold
GREY = (0.62, 0.62, 0.62)
BLACK = (0, 0, 0)

PHOTO_RECT = fitz.Rect(453.15, 157.35, 562.07, 252.06)   # do not move
HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.environ.get("SRC_TEMPLATE",
    "/dhamma/web/dipinew/sites/all/modules/dh_manageapp/templates/applicant-template.pdf")

# Page 1 keeps the original artwork -- the vector wheel logo, the Times layout and,
# critically, the passport photo cell that /dhamma/scripts/merge-img stamps at a
# fixed absolute offset. But it is recomposed as two clipped regions so the hole
# left by the three retired boxes closes up instead of sitting mid-page.
#
# Measured from the source artwork:
#   TOP     y   0.0 - 496.0   ends after "Preferred language of Instructions/Discourses:" (y1 493.1)
#   (gap)   y 496.0 - 601.8   the three cramped free-text boxes, retired to page 2
#   BOTTOM  y 601.8 - 760.0   "For Old Students" bar (y0 601.8) down to last text (y1 758.6)
#
# The bottom region slides up by SHIFT, so the freed space lands at the foot of the
# page where it is useful, and the last question gets a real answer box.
widgets = []          # (page_index, kind, name, rect, multiline)
answer_boxes = []     # (page_index, rect) - the boxes applicant answers go in

src = fitz.open(SRC)

# ---------------------------------------------------------------------------
# Repair page 1's text so it can be copied.
#
# The original's subset fonts have broken/missing ToUnicode maps, so selecting
# and copying the form gives corrupt text -- poppler silently DROPS the affected
# glyphs ("Address (with ity, ist , o ntry etc ):") and PyMuPDF mis-maps them
# ("Last Name (6Xrname)", "EMERGENCY" -> "(0(5*(1&<"). Most affected glyphs come
# out 29 codes low, one span 31 low, but the shift is per-glyph rather than
# per-span, so it cannot be reliably undone by arithmetic -- hence an explicit
# table, keyed by the span's baseline origin.
#
# The broken text is REMOVED with redaction (covering it would leave it in the
# content stream, still copyable) and redrawn in base-14 Times, which carries a
# correct encoding. Line art and the photo silhouette are preserved.
# ---------------------------------------------------------------------------
FIXES = {
    (398.1, 15.3): "For official purposes only",
    (409.9, 31.8): "Group ",     (469.4, 32.4): "Acc. ",   (320.7, 32.7): "Conf. ",
    (409.9, 43.8): "No.",        (469.4, 44.4): "No.",     (320.7, 44.7): "No.",
    (381.1, 66.6): "UDENT",      (491.7, 66.6): "UDENT",
    (52.0, 133.9): "hoto ID Type:   ",
    (187.2, 133.9): "Aadhar",    (395.2, 134.0): "ID No.:",
    (525.4, 148.7): "ber above)",
    (174.8, 164.5): "Middle",    (308.1, 165.1): "Sur",
    (108.1, 196.4): "City,",     (129.7, 196.4): "Dist.,",
    (153.2, 196.4): "Country",   (199.1, 196.4): ".)",
    (374.8, 249.0): "_",
    (454.1, 269.3): "Passport Size Photograph",
    (208.7, 273.9): "Email: ",   (72.0, 274.2): "ls",
    (94.4, 316.8): "pation:",    (348.9, 355.9): "Designation:",
    (72.3, 414.5): "EMERGENCY CONTACT ",   # trailing space: the next span abuts it
    (191.9, 414.5): "NAME & NUMBER (Also mention the relationship to the person):",
    (61.2, 444.7): " Language Comprehension: ",
    (182.3, 444.9): "How well do you understand the",
    (83.7, 455.9): "guage(s) in which this course will be conducted?",
    # the brackets around the "For Old Students" subtitle are \x0b and \x0c,
    # which extract as whitespace and vanish from a copy
    (128.6, 613.8): "(",
    (502.0, 613.8): ")",
    (154.7, 613.8): "ls of courses done in the tradition of Sayagyi U Ba Khin "
                    "as taught by S.N. Goenka",
    (110.0, 689.9): "10-day    STP ",
    (178.2, 689.9): "Special Course      20-day      30-day     45-day ",
    (398.0, 689.9): "Teacher's self course ",
    (359.2, 690.5): "60-day ",
    (497.4, 690.9): "Dhamma Service ",
    (55.6, 734.6): "4",
}

_fx = fitz.open()
_fx.insert_pdf(src, from_page=0, to_page=0)
_fp = _fx[0]

_todo, _seen = [], set()
for _b in _fp.get_text("dict")["blocks"]:
    for _l in _b.get("lines", []):
        for _s in _l["spans"]:
            _key = (round(_s["origin"][0], 1), round(_s["origin"][1], 1))
            if _key in FIXES:
                # keep the span's own colour: the "For Old Students" subtitle is
                # white on a grey bar, and redrawing it in black made it unreadable
                _c = _s["color"]
                _rgb = (((_c >> 16) & 255) / 255.0, ((_c >> 8) & 255) / 255.0, (_c & 255) / 255.0)
                _todo.append((_key, _s["origin"], _s["size"],
                              bool(_s["flags"] & 16), fitz.Rect(_s["bbox"]), _rgb))
                _seen.add(_key)
missing = set(FIXES) - _seen
if missing:
    raise SystemExit(f"FIXES keys not found in source (template changed?): {sorted(missing)}")

for _k, _origin, _size, _bold, _bbox, _rgb in _todo:
    _fp.add_redact_annot(_bbox)
_fp.apply_redactions(images=fitz.PDF_REDACT_IMAGE_NONE,
                     graphics=fitz.PDF_REDACT_LINE_ART_NONE)
for _k, _origin, _size, _bold, _bbox, _rgb in _todo:
    _txt = FIXES[_k]
    _fn = BOLD if _bold else SERIF
    # Base-14 Times is a little wider than the original subset font, so a
    # like-for-like redraw overruns its slot and collides with the neighbouring
    # span (e.g. "EMERGENCY CONTACT" running into "NAME & NUMBER", or the
    # course-type grid labels overlapping). Shrink to the width it replaced.
    _w = fitz.get_text_length(_txt, fontname=_fn, fontsize=_size)
    if _bbox.width > 1 and _w > _bbox.width:
        _size = _size * _bbox.width / _w
    _fp.insert_text(_origin, _txt, fontname=_fn, fontsize=_size, color=_rgb)

# "(Mention your ID number above)" has bbox y 140.7-150.6, straddling the 150.4
# line where the Photo ID band is cut from the identity block - the clip mangled
# it. Redraw it 1.7pt higher so it sits wholly inside the band and travels with
# it. Done here, on a normal page, because text drawn directly onto the composed
# page 1 (which is nothing but XObjects) renders as "Unknown font tag".
_capr = fitz.Rect(435, 139, 570, 152)
_fp.add_redact_annot(_capr)
_fp.apply_redactions(images=fitz.PDF_REDACT_IMAGE_NONE,
                     graphics=fitz.PDF_REDACT_LINE_ART_NONE)
_fp.insert_text((437.7, 147.0), "(Mention your ID number above)",
                fontname=BOLD, fontsize=9, color=BLACK)

src = _fx          # everything downstream now uses the repaired page

doc = fitz.open()

TOP_END, BOT_START, BOT_END = 496.0, 601.8, 760.0
SHIFT = 100.0
MOVED = ("Learn", "Reason", "MentalState")
# the course-type counter row on page 1 ("For Old Students", item 3)
COUNTERS = ("teen", "10", "stp", "spl", "20", "30", "45", "60", "tsc", "seva")

def _region(srcdoc, y0, y1, extra=None, graphics=1):
    """A one-page doc containing ONLY the content between y0 and y1.

    show_pdf_page's `clip` hides content visually but still embeds the whole
    source page in the Form XObject, so placing two clipped copies would make
    every word on page 1 extract (and copy) twice. Redaction physically removes
    the content outside the band, so each placement carries only its own region.
    """
    t = fitz.open()
    t.insert_pdf(srcdoc, from_page=0, to_page=0)
    pg = t[0]
    if y0 > 0:
        pg.add_redact_annot(fitz.Rect(0, 0, W, y0))
    if y1 < H:
        pg.add_redact_annot(fitz.Rect(0, y1, W, H))
    if extra is not None:
        pg.add_redact_annot(extra)
    # graphics=1 (REMOVE_IF_COVERED) leaves a rule alone when it belongs to a
    # path whose overall bbox reaches outside the rect - which is the case for
    # the identity box's edges. graphics=2 (REMOVE_IF_TOUCHED) is needed there.
    pg.apply_redactions(graphics=graphics)
    return t


# Page 1 bands, measured from the source artwork's horizontal rules:
#   y 114.3  Course Dates underlines
#   y 150.4  identity block TOP edge      <- photo cell 157.35-252.06 sits inside
#   y 278.8  identity block BOTTOM edge
#   y 300.5  Education underline
#
# The "Photo ID Type" row (y 120-150.4) is relocated to just under the identity
# block, which frees ~30pt of breathing space at the head of the page. The
# identity block itself CANNOT move - merge-img stamps the photo at a fixed
# absolute offset - so the questions below it shift down by the band height
# instead, and the old-students block moves up to absorb it.
HDR_SHIFT = 30.0                        # header dropped to open a top margin
IDBAND_TOP, IDBAND_BOT = 120.0, 150.4
IDENT_BOT = 282.0                       # between the 278.8 rule and Education at 285
IDBAND_H = IDBAND_BOT - IDBAND_TOP      # 30.4
IDBAND_TO = IDENT_BOT                   # band's new top
QSHIFT = IDBAND_H                       # questions 1-6 move down by that much
SHIFT = 71.8                            # old-students block moves up (was 100)

_a = _region(src, 0, IDBAND_TOP)           # header, course dates      - unmoved
# The identity block's top rule (y 150.4-152.6, 2.2pt thick) begins exactly on
# the band's cut line, so the clip keeps it and it travels down with the row -
# drawing a heavy separator between the Photo ID row and Education. It is still
# present in _c where it belongs, so drop it from the band. The rect starts at
# 150.2, clear of the "(Mention your ID number above)" caption which ends at 150.1.
_RULE = fitz.Rect(40, 150.2, 575, 153.5)
_b = _region(src, IDBAND_TOP, IDBAND_BOT, extra=_RULE,
             graphics=fitz.PDF_REDACT_LINE_ART_REMOVE_IF_TOUCHED)  # Photo ID row
_c = _region(src, IDBAND_BOT, IDENT_BOT)   # identity block + photo    - unmoved
_d = _region(src, IDENT_BOT, TOP_END)      # questions 1-6             - down QSHIFT
_e = _region(src, BOT_START, BOT_END)      # old students              - up SHIFT

_p1 = doc.new_page(width=W, height=H)
_p1.show_pdf_page(fitz.Rect(0, HDR_SHIFT, W, H + HDR_SHIFT), _a, 0)
_p1.show_pdf_page(fitz.Rect(0, 0, W, H), _c, 0)
_p1.show_pdf_page(fitz.Rect(0, IDBAND_TO - IDBAND_TOP, W, H + IDBAND_TO - IDBAND_TOP), _b, 0)
_p1.show_pdf_page(fitz.Rect(0, QSHIFT, W, H + QSHIFT), _d, 0)
_p1.show_pdf_page(fitz.Rect(0, -SHIFT, W, H - SHIFT), _e, 0)

# Re-attach page 1's form fields: content is copied by show_pdf_page, annotations
# are not. Fields above the gap keep their place; those below move up with the art.
for _w in src[0].widgets():
    if _w.field_name in MOVED:
        continue                                   # these now live on page 2
    if _w.field_name == "If yes please give details how much time daily etc":
        continue                                   # re-created larger, below
    r = fitz.Rect(_w.rect)
    # The 10-day counter can reach three digits (someone who has sat 100+
    # courses), but its box is only 16.4pt - about two digits before the text
    # starts shrinking. Its grid cell runs x 107.5-140.1, so widen it to fill
    # the cell. x is unaffected by the band shifts, so this is safe to do here.
    # Course-type counter grid. Each box was 11.1pt tall inside a 15.9pt cell
    # (rules at y 623.8 / 639.7 composed), and that height - not the width -
    # capped the digits at 7.2pt. Using more of the cell lifts them to ~8.8pt.
    # Text widgets have no visible border and are removed after filling, so
    # this is invisible on the blank form.
    if _w.field_name in COUNTERS:
        r.y0 -= 1.3
        r.y1 += 1.1
        # 10-day is the one that realistically reaches three digits, and its
        # box was the narrowest at 16.4pt; its cell runs x 107.5-140.1.
        if _w.field_name == "10":
            r.x0, r.x1 = 110.0, 137.6
    # follow the same five bands the artwork was split into
    if r.y0 >= BOT_START:                          # old students   -> up
        _dy = -SHIFT
    elif r.y0 >= TOP_END:
        continue                                   # the retired boxes' gap
    elif r.y0 >= IDENT_BOT:                        # questions 1-6  -> down
        _dy = QSHIFT
    elif r.y0 >= IDBAND_BOT:                       # identity block -> FIXED (photo)
        _dy = 0
    elif r.y0 >= IDBAND_TOP:                       # Photo ID row   -> relocated
        _dy = IDBAND_TO - IDBAND_TOP
    else:                                          # header         -> down
        _dy = HDR_SHIFT
    r = r + (0, _dy, 0, _dy)
    widgets.append((0, "check" if _w.field_type == fitz.PDF_WIDGET_TYPE_CHECKBOX
                    else "text", _w.field_name, r,
                    bool(_w.field_flags & 4096)))

# The last question ("Have you maintained your practice...") had a single 12pt rule
# for its answer. Cover that rule and give it a proper multi-line box in the space
# the reflow just freed.
_rule_y = 757.8 - SHIFT
_p1.draw_rect(fitz.Rect(288, _rule_y - 2, 552, _rule_y + 2), color=None, fill=(1, 1, 1))
_ans_top = BOT_END - SHIFT + 4
PRACTICE_H = 37                    # hand-tuned; reduced again to free page-head room
_p1.draw_rect(fitz.Rect(L + 30, _ans_top, R, _ans_top + PRACTICE_H), color=BLACK, width=0.7)
widgets.append((0, "text", "If yes please give details how much time daily etc",
                fitz.Rect(L + 33, _ans_top + 2, R - 3, _ans_top + PRACTICE_H - 2), True))



# ---------------------------------------------------------------- helpers
def text(pg, x, y, s, size=9, font=SERIF, color=BLACK):
    pg.insert_text((x, y), s, fontname=font, fontsize=size, color=color)


def box(pg, x0, y0, x1, y1, width=0.7, fill=None):
    pg.draw_rect(fitz.Rect(x0, y0, x1, y1), color=BLACK, width=width, fill=fill)


def bar(pg, y, label, h=14):
    """Grey section header bar, as in the original."""
    pg.draw_rect(fitz.Rect(L, y, R, y + h), color=None, fill=GREY)
    text(pg, L + 5, y + h - 4, label, size=9, font=BOLD, color=(1, 1, 1))
    return y + h


def line(pg, x0, y, x1, width=0.6):
    pg.draw_line(fitz.Point(x0, y), fitz.Point(x1, y), color=BLACK, width=width)


def vline(pg, x, y0, y1, width=0.6):
    pg.draw_line(fitz.Point(x, y0), fitz.Point(x, y1), color=BLACK, width=width)


def tf(pg_i, name, x0, y0, x1, y1=None, multiline=False):
    """y1 defaults to a single 12pt line below y0."""
    if y1 is None:
        y1 = y0 + 12
    widgets.append((pg_i, "text", name, fitz.Rect(x0, y0, x1, y1), multiline))


def cb(pg_i, name, x, y, s=11, pg=None):
    """Checkbox: the square outline must be drawn as page artwork -- the widget
    alone is invisible when unticked, exactly as in the original template."""
    if pg is not None:
        pg.draw_rect(fitz.Rect(x, y, x + s, y + s), color=BLACK, width=0.7)
    widgets.append((pg_i, "check", name, fitz.Rect(x, y, x + s, y + s), False))


def labelled_box(pg, pg_i, y, label, field, height, note=None):
    """A full-width labelled answer box -- the core of the redesign."""
    text(pg, L, y + 8, label, size=9, font=BOLD)
    if note:
        text(pg, L + fitz.get_text_length(label, SERIF, 9) + 8, y + 8, note, size=7.5)
    top = y + 13
    box(pg, L, top, R, top + height)
    answer_boxes.append((pg_i, fitz.Rect(L, top, R, top + height)))
    tf(pg_i, field, L + 3, top + 2, R - 3, top + height - 2, multiline=True)
    return top + height + 11


# ================================================================= PAGE 2
p2 = doc.new_page(width=W, height=H)
P2 = 1
y = bar(p2, 30, "Your Background and Reasons for Applying")
y += 8
y = labelled_box(p2, P2, y, "1.  How did you learn about Vipassana, or who introduced you to this course?", "Learn", 46)
y = labelled_box(p2, P2, y, "2.  What is your reason for attending this course?", "Reason", 76)
y = labelled_box(p2, P2, y, "3.  How would you describe your current family relationships and mental state?", "MentalState", 86)

y = bar(p2, y + 2, "For All Students (New and old students)")
y += 10
box(p2, L, y, R, y + 40)
text(p2, L + 6, y + 12, "Vipassana is a non-sectarian technique which aims for the total eradication of mental impurities and", size=8.5, font=BOLD)
text(p2, L + 6, y + 23, "the resultant highest happiness of full liberation. Its purpose is never simply to cure diseases. But, in", size=8.5, font=BOLD)
text(p2, L + 6, y + 34, "order to facilitate a smooth transition of your course, we require the following health information.", size=8.5, font=BOLD)
y += 50

text(p2, L, y + 8, "4.  Do you have any past/present physical health conditions?", size=9, font=BOLD)
text(p2, L + 10, y + 19, "(If yes, give complete details such as medication, dosage, treatment, hospitalizations etc.)", size=8)
text(p2, 486, y + 8, "No", size=9); cb(P2, "medical_no", 504, y - 1, pg=p2)
text(p2, 526, y + 8, "Yes", size=9); cb(P2, "medical_yes", 546, y - 1, pg=p2)
top = y + 24
PHYS_H = 134                       # prod tail: 145 of 35,519 exceed 400 chars, max 2810
box(p2, L, top, R, top + PHYS_H)
answer_boxes.append((P2, fitz.Rect(L, top, R, top + PHYS_H)))
tf(P2, "physical", L + 3, top + 2, R - 3, top + PHYS_H - 2, multiline=True)
y = top + PHYS_H + 11

text(p2, L, y + 8, "5.  Do you have any past/present history of psychological treatment?", size=9, font=BOLD)
text(p2, L + 10, y + 19, "(If yes, give complete details such as medication, dosage, treatment, hospitalizations etc.)", size=8)
text(p2, 486, y + 8, "No", size=9); cb(P2, "psychological_no", 504, y - 1, pg=p2)
text(p2, 526, y + 8, "Yes", size=9); cb(P2, "psychological_yes", 546, y - 1, pg=p2)
top = y + 24
PSYCH_H = 171                      # worst field: concatenation of 3 columns, prod max 5752
box(p2, L, top, R, top + PSYCH_H)
answer_boxes.append((P2, fitz.Rect(L, top, R, top + PSYCH_H)))
tf(P2, "psychological", L + 3, top + 2, R - 3, top + PSYCH_H - 2, multiline=True)
y = top + PSYCH_H + 11


# ================================================================= PAGE 3
p3 = doc.new_page(width=W, height=H)
P3 = 2
y = bar(p3, 30, "Health Information (continued)")
y += 8

text(p3, L, y + 8, "6.  Are you now taking, or have you taken within the past two years, any prescribed", size=9, font=BOLD)
text(p3, L + 10, y + 19, "medication? (If yes, please give complete details.)", size=9, font=BOLD)
text(p3, 486, y + 8, "No", size=9); cb(P3, "medication_no", 504, y - 1, pg=p3)
text(p3, 526, y + 8, "Yes", size=9); cb(P3, "medication_yes", 546, y - 1, pg=p3)
top = y + 24
box(p3, L, top, R, top + 96)
answer_boxes.append((P3, fitz.Rect(L, top, R, top + 96)))
tf(P3, "medication", L + 3, top + 2, R - 3, top + 94, multiline=True)
y = top + 106

text(p3, L, y + 8, "7. a)  Any past addictions to Tobacco, Alcohol or Drugs? (If yes, please give details)", size=9, font=BOLD)
text(p3, 486, y + 8, "No", size=9); cb(P3, "alco_no", 504, y - 1, pg=p3)
text(p3, 526, y + 8, "Yes", size=9); cb(P3, "alco_yes", 546, y - 1, pg=p3)
top = y + 14
box(p3, L, top, R, top + 74)
answer_boxes.append((P3, fitz.Rect(L, top, R, top + 74)))
tf(P3, "addiction", L + 3, top + 2, R - 3, top + 72, multiline=True)
y = top + 84

text(p3, L, y + 8, "     b)  Any current use of Tobacco, Alcohol or Drugs? (Specify substance, frequency, last use)", size=9, font=BOLD)
text(p3, 486, y + 8, "No", size=9); cb(P3, "curr_alco_no", 504, y - 1, pg=p3)
text(p3, 526, y + 8, "Yes", size=9); cb(P3, "curr_alco_yes", 546, y - 1, pg=p3)
top = y + 14
box(p3, L, top, R, top + 74)
answer_boxes.append((P3, fitz.Rect(L, top, R, top + 74)))
tf(P3, "current use intoxicants", L + 3, top + 2, R - 3, top + 72, multiline=True)
y = top + 84

text(p3, L, y + 8, "8.  For women applicants: If pregnant, please indicate which month", size=9, font=BOLD)
text(p3, L + 10, y + 19, "(Note: due to limited medical facilities nearby, we can only accept applicants in the 4th to 7th month):", size=8)
line(p3, 430, y + 21, R); tf(P3, "Text1", 432, y + 10, 566, y + 20, multiline=True)
y += 30

text(p3, L, y + 8, "9.  Do you have any past/present experience with Reiki, spiritual healing or any other", size=9, font=BOLD)
text(p3, L + 10, y + 19, "meditation practices? If yes, please give details:", size=9, font=BOLD)
text(p3, 486, y + 8, "No", size=9); cb(P3, "meditation_no", 504, y - 1, pg=p3)
text(p3, 526, y + 8, "Yes", size=9); cb(P3, "meditation_yes", 546, y - 1, pg=p3)
top = y + 24
box(p3, L, top, R, top + 66)
answer_boxes.append((P3, fitz.Rect(L, top, R, top + 66)))
tf(P3, "reiki", L + 3, top + 2, R - 3, top + 64, multiline=True)
y = top + 76

y = labelled_box(p3, P3, y, "10.  Anything you wish to add to the above information (e.g., special needs)?", "SpecialNeeds", 76)

# Held as whole paragraphs and wrapped to fit, rather than as pre-broken lines:
# the line breaks then follow whatever DECL_SIZE is set to, instead of being
# frozen at the size they were transcribed for.
DECL = [
 "I hereby agree to set aside all past spiritual/religious practices, rites, rituals, recitation, fasting and "
 "prayers as well as any religious or spiritual objects for 10-days. All reading, writing material, mobile "
 "phones etc. should be deposited at the Course Office for 10-days.",

 "I acknowledge that I have carefully read and understood the Code of Discipline for Meditation Courses. "
 "I agree to stay on the course site and to abide by all the rules and regulations for the full duration of "
 "the course. I realize that a Vipassana meditation course is a serious undertaking that will require my "
 "full mental and physical health and I affirm that I am fit to participate in it.",

 "I fully understand that the Center does not have any medical facility and thereby the management will "
 "not be liable for the consequences arising out of any illness during the period of the course. I am joining "
 "this course on my own free will. I hereby certify that the above information is true to the best of my knowledge.",

 "In addition, I hereby consent to the storage and handling on a computer or otherwise of my above stated "
 "personally identifiable information in accordance with the Privacy Policy of the facility at which the "
 "course is being held.",
]


def wrap(txt, size, font, width):
    """Greedy word wrap to a pixel width."""
    out, cur = [], ""
    for w in txt.split():
        trial = (cur + " " + w).strip()
        if fitz.get_text_length(trial, font, size) <= width:
            cur = trial
        else:
            out.append(cur)
            cur = w
    if cur:
        out.append(cur)
    return out
# The declaration is the applicant's undertaking, so it must be comfortably
# readable rather than fine print - and it is set ENTIRELY BOLD, as in the
# original 2-page template (9 bold spans, no regular ones, at 10pt).
# 10pt here would need 12 lines / 163pt, and page 3 has ~19pt to spare, so 9pt:
# re-wrapped it comes to 10 lines / 125pt, the same height as before.
DECL_SIZE = 9.0
DECL_LEAD = DECL_SIZE * 1.26            # keeps the original line spacing ratio
DECL_GAP = 4                            # blank space between paragraphs
y += 4
for _i, _para in enumerate(DECL):
    for _ln in wrap(_para, DECL_SIZE, BOLD, R - L - 8):
        text(p3, L + 4, y, _ln, size=DECL_SIZE, font=BOLD)
        y += DECL_LEAD
    if _i < len(DECL) - 1:
        y += DECL_GAP

y += 10
box(p3, L, y, R, y + 26)
p3.draw_line(fitz.Point(360, y), fitz.Point(360, y + 26))
text(p3, L + 5, y + 17, "Signature", size=9)
text(p3, 366, y + 17, "Date", size=9)
tf(P3, "Signature", 100, y + 4, 356, y + 23)
tf(P3, "Date", 398, y + 4, 564, y + 23)


# ---------------------------------------------------------------- widgets
for pi, kind, name, rect, multiline in widgets:
    w = fitz.Widget()
    w.field_name = name
    w.rect = rect
    if kind == "text":
        w.field_type = fitz.PDF_WIDGET_TYPE_TEXT
        w.text_fontsize = 10
        if multiline:
            w.field_flags = 4096
        w.field_value = ""
    else:
        w.field_type = fitz.PDF_WIDGET_TYPE_CHECKBOX
        w.field_value = False
        # The square must come from the widget's own border: page 1 is composed
        # with show_pdf_page, which copies content but not annotations, so the
        # original squares are not carried over. Without this they are invisible
        # until ticked.
        w.border_width = 0.7
        w.border_color = (0, 0, 0)
    doc[pi].add_widget(w)

out = sys.argv[1] if len(sys.argv) > 1 else "applicant-template-3page.pdf"
doc.save(out, garbage=4, deflate=True)
doc.close()

# ---------------------------------------------------------------- verify
# Field names come from the source template itself, so this check needs no side
# files. Re-open SRC: `src` was rebound to the repaired page-1-only document.
orig = {"Text": [], "CheckBox": []}
for _pg in fitz.open(SRC):
    for _w in _pg.widgets():
        orig.setdefault(_w.field_type_string, []).append(_w.field_name)
new = fitz.open(out)
got = {"Text": [], "CheckBox": []}
for p in new:
    for w in p.widgets():
        got.setdefault(w.field_type_string, []).append(w.field_name)

print(f"written: {out}  pages={new.page_count}")

# Layout check: a label overlapping an answer box means a box grew without its
# following y-advance being updated. Caught by eye once; now checked every build.
# Only real answer boxes are considered - the intro paragraph and the signature
# block legitimately contain text.
collisions = []
for _pi, _bx in answer_boxes:
    for _b in new[_pi].get_text("dict")["blocks"]:
        for _l in _b.get("lines", []):
            for _s in _l["spans"]:
                if not _s["text"].strip():
                    continue
                _r = fitz.Rect(_s["bbox"])
                if _r.y0 > _bx.y0 + 1 and _r.y1 < _bx.y1 - 1 and _r.x0 < _bx.x1 and _r.x1 > _bx.x0:
                    collisions.append((_pi + 1, _s["text"][:44], round(_r.y0, 1)))
if collisions:
    print("LAYOUT: *** text overlapping an answer box ***")
    for _c in collisions[:8]:
        print(f"   page {_c[0]} y={_c[2]}: {_c[1]!r}")
else:
    print("LAYOUT: OK - no label overlaps an answer box")
ok = True
for kind in ("Text", "CheckBox"):
    missing = sorted(set(orig[kind]) - set(got.get(kind, [])))
    extra = sorted(set(got.get(kind, [])) - set(orig[kind]))
    dup = sorted({n for n in got.get(kind, []) if got[kind].count(n) > 1})
    print(f"{kind}: original={len(orig[kind])} new={len(got.get(kind, []))}")
    if missing: print(f"  MISSING : {missing}"); ok = False
    if extra:   print(f"  EXTRA   : {extra}"); ok = False
    if dup:     print(f"  DUPLICATE: {dup}"); ok = False
print("FIELD PARITY:", "OK - all names preserved" if ok else "*** MISMATCH ***")
