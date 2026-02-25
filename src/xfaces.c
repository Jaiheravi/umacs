/* xfaces.c -- "Face" primitives.

Copyright (C) 1993-1994, 1998-2026 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.  */

/* New face implementation by Gerd Moellmann <gerd@gnu.org>.  */

/* Faces.

   When using Emacs with X, the display style of characters can be
   changed by defining `faces'.  Each face can specify the following
   display attributes:

   1. Font family name.

   2. Font foundry name.

   3. Relative proportionate width, aka character set width or set
   width (swidth), e.g. `semi-compressed'.

   4. Font height in 1/10pt.

   5. Font weight, e.g. `bold'.

   6. Font slant, e.g. `italic'.

   7. Foreground color.

   8. Background color.

   9. Whether or not characters should be underlined, and in what color.

   10. Whether or not characters should be displayed in inverse video.

   11. A background stipple, a bitmap.

   12. Whether or not characters should be overlined, and in what color.

   13. Whether or not characters should be strike-through, and in what
   color.

   14. Whether or not a box should be drawn around characters, the box
   type, and, for simple boxes, in what color.

   15. Font-spec, or nil.  This is a special attribute.

   A font-spec is a collection of font attributes (specs).

   When this attribute is specified, the face uses a font matching
   with the specs as is except for what overwritten by the specs in
   the fontset (see below).  In addition, the other font-related
   attributes (1st thru 5th) are updated from the spec.

   On the other hand, if one of the other font-related attributes are
   specified, the corresponding specs in this attribute is set to nil.

   16. A face name or list of face names from which to inherit attributes.

   17. A fontset name.  This is another special attribute.

   A fontset is a mappings from characters to font-specs, and the
   specs overwrite the font-spec in the 14th attribute.

   18. A "distant background" color, to be used when the foreground is
   too close to the background and is hard to read.

   19. Whether to extend the face to end of line when the face
   "covers" the newline that ends the line.

   On the C level, a Lisp face is completely represented by its array
   of attributes.  In that array, the zeroth element is Qface, and the
   rest are the 19 face attributes described above.  The
   lface_attribute_index enumeration, defined on dispextern.h, with
   values given by the LFACE_*_INDEX constants, is used to reference
   the individual attributes.

   Faces are frame-local by nature because Emacs allows you to define the
   same named face (face names are symbols) differently for different
   frames.  Each frame has an alist of face definitions for all named
   faces.  The value of a named face in such an alist is a Lisp vector
   with the symbol `face' in slot 0, and a slot for each of the face
   attributes mentioned above.

   There is also a global face map `Vface_new_frame_defaults',
   containing conses of (FACE_ID . FACE_DEFINITION).  Face definitions
   from this table are used to initialize faces of newly created
   frames.

   A face doesn't have to specify all attributes.  Those not specified
   have a value of `unspecified'.  Faces specifying all attributes but
   the 14th are called `fully-specified'.


   Face merging.

   The display style of a given character in the text is determined by
   combining several faces.  This process is called `face merging'.
   Face merging combines the attributes of each of the faces being
   merged such that the attributes of the face that is merged later
   override those of a face merged earlier in the process.  In
   particular, this replaces any 'unspecified' attributes with
   non-'unspecified' values.  Also, if a face inherits from another
   (via the :inherit attribute), the attributes of the parent face,
   recursively, are applied where the inheriting face doesn't specify
   non-'unspecified' values.  Any aspect of the display style that
   isn't specified by overlays or text properties is taken from the
   'default' face.  Since it is made sure that the default face is
   always fully-specified, face merging always results in a
   fully-specified face.


   Face realization.

   After all face attributes for a character have been determined by
   merging faces of that character, that face is `realized'.  The
   realization process maps face attributes to what is physically
   available on the system where Emacs runs.  The result is a
   `realized face' in the form of a struct face which is stored in the
   face cache of the frame on which it was realized.

   Face realization is done in the context of the character to display
   because different fonts may be used for different characters.  In
   other words, for characters that have different font
   specifications, different realized faces are needed to display
   them.

   Font specification is done by fontsets.  See the comment in
   fontset.c for the details.  In the current implementation, all ASCII
   characters share the same font in a fontset.

   Faces are at first realized for ASCII characters, and, at that
   time, assigned a specific realized fontset.  Hereafter, we call
   such a face as `ASCII face'.  When a face for a multibyte character
   is realized, it inherits (thus shares) a fontset of an ASCII face
   that has the same attributes other than font-related ones.

   Thus, all realized faces have a realized fontset.


   Unibyte text.

   Unibyte text (i.e. raw 8-bit characters) is displayed with the same
   font as ASCII characters.  That is because it is expected that
   unibyte text users specify a font that is suitable both for ASCII
   and raw 8-bit characters.


   Font selection.

   Font selection tries to find the best available matching font for a
   given (character, face) combination.

   If the face specifies a fontset name, that fontset determines a
   pattern for fonts of the given character.  If the face specifies a
   font name or the other font-related attributes, a fontset is
   realized from the default fontset.  In that case, that
   specification determines a pattern for ASCII characters and the
   default fontset determines a pattern for multibyte characters.

   Available fonts on the system on which Emacs runs are then matched
   against the font pattern.  The result of font selection is the best
   match for the given face attributes in this font list.

   Font selection can be influenced by the user.

   1. The user can specify the relative importance he gives the face
   attributes width, height, weight, and slant by setting
   face-font-selection-order (faces.el) to a list of face attribute
   names.  The default is '(:width :height :weight :slant), and means
   that font selection first tries to find a good match for the font
   width specified by a face, then---within fonts with that
   width---tries to find a best match for the specified font height,
   etc.

   2. Setting face-font-family-alternatives allows the user to
   specify alternative font families to try if a family specified by a
   face doesn't exist.

   3. Setting face-font-registry-alternatives allows the user to
   specify all alternative font registries to try for a face
   specifying a registry.

   4. Setting face-ignored-fonts allows the user to ignore specific
   fonts.


   Character composition.

   Usually, the realization process is already finished when Emacs
   actually reflects the desired glyph matrix on the screen.  However,
   on displaying a composition (sequence of characters to be composed
   on the screen), a suitable font for the components of the
   composition is selected and realized while drawing them on the
   screen, i.e.  the realization process is delayed but in principle
   the same.


   Initialization of basic faces.

   The faces `default', `modeline' are considered `basic faces'.
   When redisplay happens the first time for a newly created frame,
   basic faces are realized for CHARSET_ASCII.  Frame parameters are
   used to fill in unspecified attributes of the default face.  */

#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "sysstdio.h"

#include "character.h"
#include "frame.h"
#include "lisp.h"

#include <c-ctype.h>
#include "blockinput.h"
#include "buffer.h"
#include "dispextern.h"
#include "font.h"
#include "termchar.h"
#include "window.h"

/* True if face attribute ATTR is unspecified.  */

#define UNSPECIFIEDP(ATTR) EQ(ATTR, Qunspecified)

/* True if face attribute ATTR is `ignore-defface'.  */

#define IGNORE_DEFFACE_P(ATTR) EQ(ATTR, QCignore_defface)

/* True if face attribute ATTR is `reset'.  */

#define RESET_P(ATTR) EQ(ATTR, Qreset)

/* Size of hash table of realized faces in face caches (should be a
   prime number).  */

#define FACE_CACHE_BUCKETS_SIZE 1009

char unspecified_fg[] = "unspecified-fg", unspecified_bg[] = "unspecified-bg";

/* Alist of alternative font families.  Each element is of the form
   (FAMILY FAMILY1 FAMILY2 ...).  If fonts of FAMILY can't be loaded,
   try FAMILY1, then FAMILY2, ...  */

Lisp_Object Vface_alternative_font_family_alist;

/* Alist of alternative font registries.  Each element is of the form
   (REGISTRY REGISTRY1 REGISTRY2...).  If fonts of REGISTRY can't be
   loaded, try REGISTRY1, then REGISTRY2, ...  */

Lisp_Object Vface_alternative_font_registry_alist;

/* The next ID to assign to Lisp faces.  */

static int next_lface_id;

/* A vector mapping Lisp face Id's to face names.  */

static Lisp_Object* lface_id_to_name;
static ptrdiff_t lface_id_to_name_size;

/* True means face attributes have been changed since the last
   redisplay.  Used in redisplay_internal.  */

bool face_change;

/* True means don't display bold text if a face's foreground
   and background colors are the inverse of the default colors of the
   display.   This is a kluge to suppress `bold black' foreground text
   which is hard to read on an LCD monitor.  */

static bool tty_suppress_bold_inverse_default_colors_p;

/* True means the definition of the `menu' face for new frames has
   been changed.  */

static bool menu_face_changed_default;

struct named_merge_point;

static struct face* realize_face(struct face_cache*,
                                 Lisp_Object[LFACE_VECTOR_SIZE], int);
static struct face* realize_gui_face(struct face_cache*,
                                     Lisp_Object[LFACE_VECTOR_SIZE]);
static struct face* realize_tty_face(struct face_cache*,
                                     Lisp_Object[LFACE_VECTOR_SIZE]);
static bool realize_basic_faces(struct frame*);
static bool realize_default_face(struct frame*);
static void realize_named_face(struct frame*, Lisp_Object, int);
static struct face_cache* make_face_cache(struct frame*);
static void free_face_cache(struct face_cache*);
static bool merge_face_ref(struct window* w, struct frame*, Lisp_Object,
                           Lisp_Object*, bool, struct named_merge_point*,
                           enum lface_attribute_index);
static int color_distance(Emacs_Color* x, Emacs_Color* y);

/***********************************************************************
         Frames and faces
 ***********************************************************************/

/* Initialize face cache and basic faces for frame F.  */

void init_frame_faces(struct frame* f)
{
  /* Make a face cache, if F doesn't have one.  */
  if (FRAME_FACE_CACHE(f) == NULL)
    FRAME_FACE_CACHE(f) = make_face_cache(f);

  /* Realize faces early (Bug#17889).  */
  if (!realize_basic_faces(f))
    emacs_abort();
}


/* Free face cache of frame F.  Called from frame-dependent
   resource freeing function, e.g. (x|tty)_free_frame_resources.  */

void free_frame_faces(struct frame* f)
{
  struct face_cache* face_cache = FRAME_FACE_CACHE(f);
  if (face_cache)
  {
    free_face_cache(face_cache);
    FRAME_FACE_CACHE(f) = NULL;
  }
}

/* Clear face caches, and recompute basic faces for frame F.  Call
   this after changing frame parameters on which those faces depend,
   or when realized faces have been freed due to changing attributes
   of named faces.  */

void recompute_basic_faces(struct frame* f)
{
  if (FRAME_FACE_CACHE(f))
  {
    bool non_basic_faces_cached =
        FRAME_FACE_CACHE(f)->used > BASIC_FACE_ID_SENTINEL;
    clear_face_cache(false);
    if (!realize_basic_faces(f))
      emacs_abort();
    /* The call to realize_basic_faces above recomputed the basic
       faces and freed their fontsets, but if there are non-ASCII
       faces in the cache, they might now be invalid, and they
       reference fontsets that are no longer in Vfontset_table.  We
       therefore must force complete regeneration of all frame faces.  */
    if (non_basic_faces_cached)
      f->face_change = true;
  }
}


/* Clear the face caches of all frames.  CLEAR_FONTS_P means
   try to free unused fonts, too.  */

void clear_face_cache(bool clear_fonts_p) {}

DEFUN ("clear-face-cache", Fclear_face_cache, Sclear_face_cache, 0, 1, 0,
       doc: /* Clear face caches on all frames.
Optional THOROUGHLY non-nil means try to free unused fonts, too.  */)
(Lisp_Object thoroughly)
{
  clear_face_cache(!NILP(thoroughly));
  face_change = true;
  windows_or_buffers_changed = 53;
  return Qnil;
}


/***********************************************************************
                            Color Handling
 ***********************************************************************/

/* Parse hex color component specification that starts at S and ends
   right before E.  Set *DST to the parsed value normalized so that
   the maximum value for the number of hex digits given becomes 65535,
   and return true on success, false otherwise.  */
static bool parse_hex_color_comp(const char* s, const char* e,
                                 unsigned short* dst)
{
  int n = e - s;
  if (n <= 0 || n > 4)
    return false;
  int val = 0;
  for (; s < e; s++)
  {
    int digit;
    if (*s >= '0' && *s <= '9')
      digit = *s - '0';
    else if (*s >= 'A' && *s <= 'F')
      digit = *s - 'A' + 10;
    else if (*s >= 'a' && *s <= 'f')
      digit = *s - 'a' + 10;
    else
      return false;
    val = (val << 4) | digit;
  }
  int maxval = (1 << (n * 4)) - 1;
  *dst = (unsigned)val * 65535 / maxval;
  return true;
}

/* Parse floating-point color component specification that starts at S
   and ends right before E.  Put the integer near-equivalent of that
   into *DST.  Return true if successful, false otherwise.  */
static bool parse_float_color_comp(const char* s, const char* e,
                                   unsigned short* dst)
{
  /* Only allow decimal float literals without whitespace.  */
  for (const char* p = s; p < e; p++)
    if (!((*p >= '0' && *p <= '9') || *p == '.' || *p == '+' || *p == '-' ||
          *p == 'e' || *p == 'E'))
      return false;
  char* end;
  double x = strtod(s, &end);
  if (end == e && 0 <= x && x <= 1)
  {
    *dst = lrint(x * 65535);
    return true;
  }
  else
    return false;
}

/* Parse SPEC as a numeric color specification and set *R, *G and *B.
   Return true on success, false on failure.

   Recognized formats of SPEC:

    "#RGB", with R, G and B hex strings of equal length, 1-4 digits each.
    "rgb:R/G/B", with R, G and B hex strings, 1-4 digits each.
    "rgbi:R/G/B", with R, G and B numbers in [0,1].

   If the function succeeds, it assigns to each of the components *R,
   *G, and *B a value normalized to be in the [0, 65535] range.  If
   the function fails, some or all of the components remain unassigned.  */
bool parse_color_spec(const char* spec, unsigned short* r, unsigned short* g,
                      unsigned short* b)
{
  int len = strlen(spec);
  if (spec[0] == '#')
  {
    if ((len - 1) % 3 == 0)
    {
      int n = (len - 1) / 3;
      return (parse_hex_color_comp(spec + 1 + 0 * n, spec + 1 + 1 * n, r) &&
              parse_hex_color_comp(spec + 1 + 1 * n, spec + 1 + 2 * n, g) &&
              parse_hex_color_comp(spec + 1 + 2 * n, spec + 1 + 3 * n, b));
    }
  }
  else if (strncmp(spec, "rgb:", 4) == 0)
  {
    char const* sep1 = strchr(spec + 4, '/');
    if (!sep1)
      return false;
    char const* sep2 = strchr(sep1 + 1, '/');
    return (sep2 && parse_hex_color_comp(spec + 4, sep1, r) &&
            parse_hex_color_comp(sep1 + 1, sep2, g) &&
            parse_hex_color_comp(sep2 + 1, spec + len, b));
  }
  else if (strncmp(spec, "rgbi:", 5) == 0)
  {
    char const* sep1 = strchr(spec + 5, '/');
    if (!sep1)
      return false;
    char const* sep2 = strchr(sep1 + 1, '/');
    return (sep2 && parse_float_color_comp(spec + 5, sep1, r) &&
            parse_float_color_comp(sep1 + 1, sep2, g) &&
            parse_float_color_comp(sep2 + 1, spec + len, b));
  }
  return false;
}

DEFUN ("color-values-from-color-spec",
       Fcolor_values_from_color_spec,
       Scolor_values_from_color_spec,
       1, 1, 0,
       doc: /* Parse color SPEC as a numeric color and return (RED GREEN BLUE).
This function recognizes the following formats for SPEC:

 #RGB, where R, G and B are hex numbers of equal length, 1-4 digits each.
 rgb:R/G/B, where R, G, and B are hex numbers, 1-4 digits each.
 rgbi:R/G/B, where R, G and B are floating-point numbers in [0,1].

If SPEC is not in one of the above forms, return nil.

Each of the 3 integer members of the resulting list, RED, GREEN, and BLUE,
is normalized to have its value in [0,65535].  */)
(Lisp_Object spec)
{
  CHECK_STRING(spec);
  unsigned short r, g, b;
  return (parse_color_spec(SSDATA(spec), &r, &g, &b) ? list3i(r, g, b) : Qnil);
}

/* Parse RGB_LIST, and fill in the RGB fields of COLOR.
   RGB_LIST should contain (at least) 3 lisp integers.
   Return true iff RGB_LIST is OK.  */

static bool parse_rgb_list(Lisp_Object rgb_list, Emacs_Color* color)
{
#define PARSE_RGB_LIST_FIELD(field)                                            \
  if (CONSP(rgb_list) && FIXNUMP(XCAR(rgb_list)))                              \
  {                                                                            \
    color->field = XFIXNUM(XCAR(rgb_list));                                    \
    rgb_list = XCDR(rgb_list);                                                 \
  }                                                                            \
  else                                                                         \
    return false;

  PARSE_RGB_LIST_FIELD(red);
  PARSE_RGB_LIST_FIELD(green);
  PARSE_RGB_LIST_FIELD(blue);

  return true;
}


/* Lookup on frame F the color described by the lisp string COLOR.
   The resulting tty color is returned in TTY_COLOR; if STD_COLOR is
   non-zero, then the `standard' definition of the same color is
   returned in it.  */

static bool tty_lookup_color(struct frame* f, Lisp_Object color,
                             Emacs_Color* tty_color, Emacs_Color* std_color)
{
  Lisp_Object frame, color_desc;

  if (!STRINGP(color) || NILP(Ffboundp(Qtty_color_desc)))
    return false;

  XSETFRAME(frame, f);

  color_desc = calln(Qtty_color_desc, color, frame);
  if (CONSP(color_desc) && CONSP(XCDR(color_desc)))
  {
    Lisp_Object rgb;

    if (!FIXNUMP(XCAR(XCDR(color_desc))))
      return false;

    tty_color->pixel = XFIXNUM(XCAR(XCDR(color_desc)));

    rgb = XCDR(XCDR(color_desc));
    if (!parse_rgb_list(rgb, tty_color))
      return false;

    /* Should we fill in STD_COLOR too?  */
    if (std_color)
    {
      /* Default STD_COLOR to the same as TTY_COLOR.  */
      *std_color = *tty_color;

      /* Do a quick check to see if the returned descriptor is
         actually _exactly_ equal to COLOR, otherwise we have to
         lookup STD_COLOR separately.  If it's impossible to lookup
         a standard color, we just give up and use TTY_COLOR.  */
      if ((!STRINGP(XCAR(color_desc)) ||
           NILP(Fstring_equal(color, XCAR(color_desc)))) &&
          !NILP(Ffboundp(Qtty_color_standard_values)))
      {
        /* Look up STD_COLOR separately.  */
        rgb = calln(Qtty_color_standard_values, color);
        if (!parse_rgb_list(rgb, std_color))
          return false;
      }
    }

    return true;
  }
  else if (NILP(Fsymbol_value(Qtty_defined_color_alist)))
    /* We were called early during startup, and the colors are not
       yet set up in tty-defined-color-alist.  Don't return a failure
       indication, since this produces the annoying "Unable to
       load color" messages in the *Messages* buffer.  */
    return true;
  else
    /* tty-color-desc seems to have returned a bad value.  */
    return false;
}

/* An implementation of defined_color_hook for tty frames.  */

bool tty_defined_color(struct frame* f, const char* color_name,
                       Emacs_Color* color_def, bool alloc, bool _makeIndex)
{
  bool status = true;

  /* Defaults.  */
  color_def->pixel = FACE_TTY_DEFAULT_COLOR;
  color_def->red = 0;
  color_def->blue = 0;
  color_def->green = 0;

  if (*color_name)
  {
    Lisp_Object lcolor = build_string(color_name);
    status = tty_lookup_color(f, lcolor, color_def, NULL);

    if (color_def->pixel == FACE_TTY_DEFAULT_COLOR)
    {
      color_name = SSDATA(lcolor);
      if (strcmp(color_name, "unspecified-fg") == 0)
        color_def->pixel = FACE_TTY_DEFAULT_FG_COLOR;
      else if (strcmp(color_name, "unspecified-bg") == 0)
        color_def->pixel = FACE_TTY_DEFAULT_BG_COLOR;
    }
  }

  if (color_def->pixel != FACE_TTY_DEFAULT_COLOR)
    status = true;

  return status;
}

/* Given the index IDX of a tty color on frame F, return its name, a
   Lisp string.  */

Lisp_Object tty_color_name(struct frame* f, int idx)
{
  if (idx >= 0 && !NILP(Ffboundp(Qtty_color_by_index)))
  {
    Lisp_Object frame;
    Lisp_Object coldesc;

    XSETFRAME(frame, f);
    coldesc = calln(Qtty_color_by_index, make_fixnum(idx), frame);

    if (!NILP(coldesc))
      return XCAR(coldesc);
  }

  if (idx == FACE_TTY_DEFAULT_FG_COLOR)
    return build_string(unspecified_fg);
  if (idx == FACE_TTY_DEFAULT_BG_COLOR)
    return build_string(unspecified_bg);

  return Qunspecified;
}


/* Return true if COLOR_NAME is a shade of gray (or white or
   black) on frame F.

   The criterion implemented here is not a terribly sophisticated one.  */

static bool face_color_gray_p(struct frame* f, const char* color_name)
{
  Emacs_Color color;
  bool gray_p;

  if (FRAME_TERMINAL(f)->defined_color_hook(f, color_name, &color, false, true))
    gray_p =
        (/* Any color sufficiently close to black counts as gray.  */
         (color.red < 5000 && color.green < 5000 && color.blue < 5000) ||
         ((eabs(color.red - color.green) < max(color.red, color.green) / 20) &&
          (eabs(color.green - color.blue) <
           max(color.green, color.blue) / 20) &&
          (eabs(color.blue - color.red) < max(color.blue, color.red) / 20)));
  else
    gray_p = false;

  return gray_p;
}


/* Return true if color COLOR_NAME can be displayed on frame F.
   BACKGROUND_P means the color will be used as background color.  */

static bool face_color_supported_p(struct frame* f, const char* color_name,
                                   bool background_p)
{
  Lisp_Object frame;
  Emacs_Color not_used;

  XSETFRAME(frame, f);
  return tty_defined_color(f, color_name, &not_used, false, false);
}


DEFUN ("color-gray-p", Fcolor_gray_p, Scolor_gray_p, 1, 2, 0,
       doc: /* Return non-nil if COLOR is a shade of gray (or white or black).
FRAME specifies the frame and thus the display for interpreting COLOR.
If FRAME is nil or omitted, use the selected frame.  */)
(Lisp_Object color, Lisp_Object frame)
{
  CHECK_STRING(color);
  return (face_color_gray_p(decode_any_frame(frame), SSDATA(color)) ? Qt
                                                                    : Qnil);
}


DEFUN ("color-supported-p", Fcolor_supported_p,
       Scolor_supported_p, 1, 3, 0,
       doc: /* Return non-nil if COLOR can be displayed on FRAME.
BACKGROUND-P non-nil means COLOR is used as a background.
Otherwise, this function tells whether it can be used as a foreground.
If FRAME is nil or omitted, use the selected frame.
COLOR must be a valid color name.  */)
(Lisp_Object color, Lisp_Object frame, Lisp_Object background_p)
{
  CHECK_STRING(color);
  return (face_color_supported_p(decode_any_frame(frame), SSDATA(color),
                                 !NILP(background_p))
              ? Qt
              : Qnil);
}


static unsigned long load_color2(struct frame* f, struct face* face,
                                 Lisp_Object name,
                                 enum lface_attribute_index target_index,
                                 Emacs_Color* color)
{
  eassert(STRINGP(name));
  eassert(target_index == LFACE_FOREGROUND_INDEX ||
          target_index == LFACE_BACKGROUND_INDEX ||
          target_index == LFACE_UNDERLINE_INDEX ||
          target_index == LFACE_OVERLINE_INDEX ||
          target_index == LFACE_STRIKE_THROUGH_INDEX ||
          target_index == LFACE_BOX_INDEX);

  /* if the color map is full, defined_color_hook will return a best match
     to the values in an existing cell. */
  if (!FRAME_TERMINAL(f)->defined_color_hook(f, SSDATA(name), color, true,
                                             true))
  {
    add_to_log("Unable to load color \"%s\"", name);

    switch (target_index)
    {
    case LFACE_FOREGROUND_INDEX:
      face->foreground_defaulted_p = true;
      color->pixel = FRAME_FOREGROUND_PIXEL(f);
      break;

    case LFACE_BACKGROUND_INDEX:
      face->background_defaulted_p = true;
      color->pixel = FRAME_BACKGROUND_PIXEL(f);
      break;

    case LFACE_UNDERLINE_INDEX:
      face->underline_defaulted_p = true;
      color->pixel = FRAME_FOREGROUND_PIXEL(f);
      break;

    case LFACE_OVERLINE_INDEX:
      face->overline_color_defaulted_p = true;
      color->pixel = FRAME_FOREGROUND_PIXEL(f);
      break;

    case LFACE_STRIKE_THROUGH_INDEX:
      face->strike_through_color_defaulted_p = true;
      color->pixel = FRAME_FOREGROUND_PIXEL(f);
      break;

    case LFACE_BOX_INDEX:
      face->box_color_defaulted_p = true;
      color->pixel = FRAME_FOREGROUND_PIXEL(f);
      break;

    default:
      emacs_abort();
    }
  }
#ifdef GLYPH_DEBUG
  else
    ++ncolors_allocated;
#endif

  return color->pixel;
}

/* Load color with name NAME for use by face FACE on frame F.
   TARGET_INDEX must be one of LFACE_FOREGROUND_INDEX,
   LFACE_BACKGROUND_INDEX, LFACE_UNDERLINE_INDEX, LFACE_OVERLINE_INDEX,
   LFACE_STRIKE_THROUGH_INDEX, or LFACE_BOX_INDEX.  Value is the
   pixel color.  If color cannot be loaded, display a message, and
   return the foreground, background or underline color of F, but
   record that fact in flags of the face so that we don't try to free
   these colors.  */

#ifndef MSDOS
static
#endif
    unsigned long load_color(struct frame* f, struct face* face,
                             Lisp_Object name,
                             enum lface_attribute_index target_index)
{
  Emacs_Color color;
  return load_color2(f, face, name, target_index, &color);
}

/***********************************************************************
         XLFD Font Names
 ***********************************************************************/

/* An enumerator for each field of an XLFD font name.  */

enum xlfd_field
{
  XLFD_FOUNDRY,
  XLFD_FAMILY,
  XLFD_WEIGHT,
  XLFD_SLANT,
  XLFD_SWIDTH,
  XLFD_ADSTYLE,
  XLFD_PIXEL_SIZE,
  XLFD_POINT_SIZE,
  XLFD_RESX,
  XLFD_RESY,
  XLFD_SPACING,
  XLFD_AVGWIDTH,
  XLFD_REGISTRY,
  XLFD_ENCODING,
  XLFD_LAST
};

/* Order by which font selection chooses fonts.  The default values
   mean "first, find a best match for the font width, then for the
   font height, then for weight, then for slant."  This variable can be
   set via 'internal-set-font-selection-order'.  */

static int font_sort_order[4];


/***********************************************************************
            Lisp Faces
 ***********************************************************************/

/* Access face attributes of face LFACE, a Lisp vector.  */

#define LFACE_FAMILY(LFACE) AREF(LFACE, LFACE_FAMILY_INDEX)
#define LFACE_FOUNDRY(LFACE) AREF(LFACE, LFACE_FOUNDRY_INDEX)
#define LFACE_HEIGHT(LFACE) AREF(LFACE, LFACE_HEIGHT_INDEX)
#define LFACE_WEIGHT(LFACE) AREF(LFACE, LFACE_WEIGHT_INDEX)
#define LFACE_SLANT(LFACE) AREF(LFACE, LFACE_SLANT_INDEX)
#define LFACE_UNDERLINE(LFACE) AREF(LFACE, LFACE_UNDERLINE_INDEX)
#define LFACE_INVERSE(LFACE) AREF(LFACE, LFACE_INVERSE_INDEX)
#define LFACE_FOREGROUND(LFACE) AREF(LFACE, LFACE_FOREGROUND_INDEX)
#define LFACE_BACKGROUND(LFACE) AREF(LFACE, LFACE_BACKGROUND_INDEX)
#define LFACE_STIPPLE(LFACE) AREF(LFACE, LFACE_STIPPLE_INDEX)
#define LFACE_SWIDTH(LFACE) AREF(LFACE, LFACE_SWIDTH_INDEX)
#define LFACE_OVERLINE(LFACE) AREF(LFACE, LFACE_OVERLINE_INDEX)
#define LFACE_STRIKE_THROUGH(LFACE) AREF(LFACE, LFACE_STRIKE_THROUGH_INDEX)
#define LFACE_BOX(LFACE) AREF(LFACE, LFACE_BOX_INDEX)
#define LFACE_FONT(LFACE) AREF(LFACE, LFACE_FONT_INDEX)
#define LFACE_INHERIT(LFACE) AREF(LFACE, LFACE_INHERIT_INDEX)
#define LFACE_FONTSET(LFACE) AREF(LFACE, LFACE_FONTSET_INDEX)
#define LFACE_EXTEND(LFACE) AREF(LFACE, LFACE_EXTEND_INDEX)
#define LFACE_DISTANT_FOREGROUND(LFACE)                                        \
  AREF(LFACE, LFACE_DISTANT_FOREGROUND_INDEX)

/* True if LFACE is a Lisp face.  A Lisp face is a vector of size
   LFACE_VECTOR_SIZE which has the symbol `face' in slot 0.  */

#define LFACEP(LFACE)                                                          \
  (VECTORP(LFACE) && ASIZE(LFACE) == LFACE_VECTOR_SIZE &&                      \
   EQ(AREF(LFACE, 0), Qface))


/* Face attribute symbols for each value of LFACE_*_INDEX.  */
static Lisp_Object face_attr_sym[LFACE_VECTOR_SIZE];

#define check_lface_attrs(attrs) (void)0
#define check_lface(lface) (void)0


/* Face-merge cycle checking.  */

enum named_merge_point_kind
{
  NAMED_MERGE_POINT_NORMAL,
  NAMED_MERGE_POINT_REMAP
};

/* A `named merge point' is simply a point during face-merging where we
   look up a face by name.  We keep a stack of which named lookups we're
   currently processing so that we can easily detect cycles, using a
   linked- list of struct named_merge_point structures, typically
   allocated on the stack frame of the named lookup functions which are
   active (so no consing is required).  */
struct named_merge_point
{
  Lisp_Object face_name;
  enum named_merge_point_kind named_merge_point_kind;
  struct named_merge_point* prev;
};


/* If a face merging cycle is detected for FACE_NAME, return false,
   otherwise add NEW_NAMED_MERGE_POINT, which is initialized using
   FACE_NAME and NAMED_MERGE_POINT_KIND, as the head of the linked list
   pointed to by NAMED_MERGE_POINTS, and return true.  */

static bool
push_named_merge_point(struct named_merge_point* new_named_merge_point,
                       Lisp_Object face_name,
                       enum named_merge_point_kind named_merge_point_kind,
                       struct named_merge_point** named_merge_points)
{
  struct named_merge_point* prev;

  for (prev = *named_merge_points; prev; prev = prev->prev)
    if (EQ(face_name, prev->face_name))
    {
      if (prev->named_merge_point_kind == named_merge_point_kind)
        /* A cycle, so fail.  */
        return false;
      else if (prev->named_merge_point_kind == NAMED_MERGE_POINT_REMAP)
        /* A remap `hides ' any previous normal merge points
           (because the remap means that it's actually different face),
           so as we know the current merge point must be normal, we
           can just assume it's OK.  */
        break;
    }

  new_named_merge_point->face_name = face_name;
  new_named_merge_point->named_merge_point_kind = named_merge_point_kind;
  new_named_merge_point->prev = *named_merge_points;

  *named_merge_points = new_named_merge_point;

  return true;
}


/* Resolve face name FACE_NAME.  If FACE_NAME is a string, intern it
   to make it a symbol.  If FACE_NAME is an alias for another face,
   return that face's name.

   Return default face in case of errors.  */

static Lisp_Object resolve_face_name(Lisp_Object face_name, bool signal_p)
{
  Lisp_Object orig_face;
  Lisp_Object tortoise, hare;

  if (STRINGP(face_name))
    face_name = Fintern(face_name, Qnil);

  if (NILP(face_name) || !SYMBOLP(face_name))
    return face_name;

  orig_face = face_name;
  tortoise = hare = face_name;

  while (true)
  {
    face_name = hare;
    hare = Fget(hare, Qface_alias);
    if (NILP(hare) || !SYMBOLP(hare))
      break;

    face_name = hare;
    hare = Fget(hare, Qface_alias);
    if (NILP(hare) || !SYMBOLP(hare))
      break;

    tortoise = Fget(tortoise, Qface_alias);
    if (BASE_EQ(hare, tortoise))
    {
      if (signal_p)
        circular_list(orig_face);
      return Qdefault;
    }
  }

  return face_name;
}


/* Return the face definition of FACE_NAME on frame F.  F null means
   return the definition for new frames.  FACE_NAME may be a string or
   a symbol (apparently Emacs 20.2 allowed strings as face names in
   face text properties; Ediff uses that).
   If SIGNAL_P, signal an error if FACE_NAME is not a valid face name.
   Otherwise, value is nil if FACE_NAME is not a valid face name.  */
static Lisp_Object lface_from_face_name_no_resolve(struct frame* f,
                                                   Lisp_Object face_name,
                                                   bool signal_p)
{
  Lisp_Object lface;

  if (f)
    lface = Fgethash(face_name, f->face_hash_table, Qnil);
  else
    lface = CDR(Fgethash(face_name, Vface_new_frame_defaults, Qnil));

  if (signal_p && NILP(lface))
    signal_error("Invalid face", face_name);

  check_lface(lface);

  return lface;
}

/* Return the face definition of FACE_NAME on frame F.  F null means
   return the definition for new frames.  FACE_NAME may be a string or
   a symbol (apparently Emacs 20.2 allowed strings as face names in
   face text properties; Ediff uses that).  If FACE_NAME is an alias
   for another face, return that face's definition.
   If SIGNAL_P, signal an error if FACE_NAME is not a valid face name.
   Otherwise, value is nil if FACE_NAME is not a valid face name.  */
static Lisp_Object lface_from_face_name(struct frame* f, Lisp_Object face_name,
                                        bool signal_p)
{
  face_name = resolve_face_name(face_name, signal_p);
  return lface_from_face_name_no_resolve(f, face_name, signal_p);
}


/* Get face attributes of face FACE_NAME from frame-local faces on
   frame F.  Store the resulting attributes in ATTRS which must point
   to a vector of Lisp_Objects of size LFACE_VECTOR_SIZE.
   If SIGNAL_P, signal an error if FACE_NAME does not name a face.
   Otherwise, return true iff FACE_NAME is a face.  */

static bool get_lface_attributes_no_remap(struct frame* f,
                                          Lisp_Object face_name,
                                          Lisp_Object attrs[LFACE_VECTOR_SIZE],
                                          bool signal_p)
{
  Lisp_Object lface;

  lface = lface_from_face_name_no_resolve(f, face_name, signal_p);

  if (!NILP(lface))
    memcpy(attrs, xvector_contents(lface), LFACE_VECTOR_SIZE * sizeof *attrs);

  return !NILP(lface);
}

/* Get face attributes of face FACE_NAME from frame-local faces on
   frame F.  Store the resulting attributes in ATTRS which must point
   to a vector of Lisp_Objects of size LFACE_VECTOR_SIZE.
   If FACE_NAME is an alias for another face, use that face's
   definition.  If SIGNAL_P, signal an error if FACE_NAME does not
   name a face.  Otherwise, return true iff FACE_NAME is a face.  If W
   is non-NULL, also consider remappings attached to the window.
   */
static bool get_lface_attributes(struct window* w, struct frame* f,
                                 Lisp_Object face_name,
                                 Lisp_Object attrs[LFACE_VECTOR_SIZE],
                                 bool signal_p,
                                 struct named_merge_point* named_merge_points)
{
  Lisp_Object face_remapping;
  eassert(w == NULL || WINDOW_XFRAME(w) == f);

  face_name = resolve_face_name(face_name, signal_p);

  /* See if SYMBOL has been remapped to some other face (usually this is
     done buffer-locally).  We only do that of F is non-NULL, because
     face remapping is not relevant for default attributes of faces for
     future frames, and because merge_face_ref cannot handle NULL frames
     anyway.  */
  if (f)
  {
    face_remapping = assq_no_quit(face_name, Vface_remapping_alist);
    if (CONSP(face_remapping))
    {
      struct named_merge_point named_merge_point;

      if (push_named_merge_point(&named_merge_point, face_name,
                                 NAMED_MERGE_POINT_REMAP, &named_merge_points))
      {
        int i;

        for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
          attrs[i] = Qunspecified;

        return merge_face_ref(w, f, XCDR(face_remapping), attrs, signal_p,
                              named_merge_points, 0);
      }
    }
  }

  /* Default case, no remapping.  */
  return get_lface_attributes_no_remap(f, face_name, attrs, signal_p);
}


/* True iff all attributes in face attribute vector ATTRS are
   specified, i.e. are non-nil.  */

static bool lface_fully_specified_p(Lisp_Object attrs[LFACE_VECTOR_SIZE])
{
  int i;

  for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
    if (i != LFACE_FONT_INDEX && i != LFACE_INHERIT_INDEX &&
        i != LFACE_DISTANT_FOREGROUND_INDEX)
      if ((UNSPECIFIEDP(attrs[i]) || IGNORE_DEFFACE_P(attrs[i])))
        break;

  return i == LFACE_VECTOR_SIZE;
}


/* Merges the face height FROM with the face height TO, and returns the
   merged height.  If FROM is an invalid height, then INVALID is
   returned instead.  FROM and TO may be either absolute face heights or
   `relative' heights; the returned value is always an absolute height
   unless both FROM and TO are relative.  */

static Lisp_Object merge_face_heights(Lisp_Object from, Lisp_Object to,
                                      Lisp_Object invalid)
{
  Lisp_Object result = invalid;

  if (FIXNUMP(from))
    /* FROM is absolute, just use it as is.  */
    result = from;
  else if (FLOATP(from))
  /* FROM is a scale, use it to adjust TO.  */
  {
    if (FIXNUMP(to))
      /* relative X absolute => absolute */
      result = make_fixnum(XFLOAT_DATA(from) * XFIXNUM(to));
    else if (FLOATP(to))
      /* relative X relative => relative */
      result = make_float(XFLOAT_DATA(from) * XFLOAT_DATA(to));
    else if (UNSPECIFIEDP(to))
      result = from;
  }
  else if (FUNCTIONP(from))
  /* FROM is a function, which use to adjust TO.  */
  {
    /* Call function with current height as argument.
 From is the new height.  */
    result = safe_calln(from, to);

    /* Ensure that if TO was absolute, so is the result.  */
    if (FIXNUMP(to) && !FIXNUMP(result))
      result = invalid;
  }

  return result;
}


/* Merge two Lisp face attribute vectors on frame F, FROM and TO, and
   store the resulting attributes in TO, which must be already be
   completely specified and contain only absolute attributes.  Every
   specified attribute of FROM overrides the corresponding attribute of
   TO; merge relative attributes in FROM with the absolute value in TO,
   which attributes also replace it.  Use NAMED_MERGE_POINTS internally
   to detect loops in face inheritance/remapping; it should be 0 when
   called from other places.  If window W is non-NULL, use W to
   interpret face specifications. */
static void merge_face_vectors(struct window* w, struct frame* f,
                               const Lisp_Object* from, Lisp_Object* to,
                               struct named_merge_point* named_merge_points)
{
  int i;
  Lisp_Object font = Qnil, tospec, adstyle;

  /* If FROM inherits from some other faces, merge their attributes into
     TO before merging FROM's direct attributes.  Note that an :inherit
     attribute of `unspecified' is the same as one of nil; we never
     merge :inherit attributes, so nil is more correct, but lots of
     other code uses `unspecified' as a generic value for face attributes. */
  if (!UNSPECIFIEDP(from[LFACE_INHERIT_INDEX]) &&
      !NILP(from[LFACE_INHERIT_INDEX]))
    merge_face_ref(w, f, from[LFACE_INHERIT_INDEX], to, false,
                   named_merge_points, 0);

  if (FONT_SPEC_P(from[LFACE_FONT_INDEX]))
  {
    if (!UNSPECIFIEDP(to[LFACE_FONT_INDEX]))
      font = merge_font_spec(from[LFACE_FONT_INDEX], to[LFACE_FONT_INDEX]);
    else
      font = copy_font_spec(from[LFACE_FONT_INDEX]);
    to[LFACE_FONT_INDEX] = font;
  }

  for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
    if (!UNSPECIFIEDP(from[i]))
    {
      if (i == LFACE_HEIGHT_INDEX && !FIXNUMP(from[i]))
      {
        to[i] = merge_face_heights(from[i], to[i], to[i]);
        font_clear_prop(to, FONT_SIZE_INDEX);
      }
      else if (i != LFACE_FONT_INDEX && !EQ(to[i], from[i]))
      {
        to[i] = from[i];
        if (i >= LFACE_FAMILY_INDEX && i <= LFACE_SLANT_INDEX)
          font_clear_prop(to,
                          (i == LFACE_FAMILY_INDEX        ? FONT_FAMILY_INDEX
                               : i == LFACE_FOUNDRY_INDEX ? FONT_FOUNDRY_INDEX
                               : i == LFACE_SWIDTH_INDEX  ? FONT_WIDTH_INDEX
                               : i == LFACE_HEIGHT_INDEX  ? FONT_SIZE_INDEX
                               : i == LFACE_WEIGHT_INDEX  ? FONT_WEIGHT_INDEX
                                                          : FONT_SLANT_INDEX));
      }
    }

  /* If FROM specifies a font spec, make its contents take precedence
     over :family and other attributes.  This is needed for face
     remapping using :font to work.  */

  if (!NILP(font))
  {
    if (!NILP(AREF(font, FONT_FOUNDRY_INDEX)))
      to[LFACE_FOUNDRY_INDEX] = SYMBOL_NAME(AREF(font, FONT_FOUNDRY_INDEX));
    if (!NILP(AREF(font, FONT_FAMILY_INDEX)))
      to[LFACE_FAMILY_INDEX] = SYMBOL_NAME(AREF(font, FONT_FAMILY_INDEX));
    if (!NILP(AREF(font, FONT_WEIGHT_INDEX)))
      to[LFACE_WEIGHT_INDEX] = FONT_WEIGHT_FOR_FACE(font);
    if (!NILP(AREF(font, FONT_SLANT_INDEX)))
      to[LFACE_SLANT_INDEX] = FONT_SLANT_FOR_FACE(font);
    if (!NILP(AREF(font, FONT_WIDTH_INDEX)))
      to[LFACE_SWIDTH_INDEX] = FONT_WIDTH_FOR_FACE(font);

    if (!NILP(AREF(font, FONT_ADSTYLE_INDEX)))
    {
      /* If an adstyle is specified in FROM's font spec, create a
         font spec for TO if none exists, and transfer the adstyle
         there.  */

      tospec = to[LFACE_FONT_INDEX];
      adstyle = AREF(font, FONT_ADSTYLE_INDEX);

      if (!NILP(tospec))
        tospec = copy_font_spec(tospec);
      else
        tospec = Ffont_spec(0, NULL);

      to[LFACE_FONT_INDEX] = tospec;
      ASET(tospec, FONT_ADSTYLE_INDEX, adstyle);
    }

    ASET(font, FONT_SIZE_INDEX, Qnil);
  }

  /* TO is always an absolute face, which should inherit from nothing.
     We blindly copy the :inherit attribute above and fix it up here.  */
  to[LFACE_INHERIT_INDEX] = Qnil;
}

/* Chase the chain of face inheritance of frame F's face whose
   attributes are in ATTRS, for a non-'unspecified' value of face
   attribute whose index is ATTR_IDX, and return that value.  Window
   W, if non-NULL, is used to filter face specifications.  */
static Lisp_Object
face_inherited_attr(struct window* w, struct frame* f,
                    Lisp_Object attrs[LFACE_VECTOR_SIZE],
                    enum lface_attribute_index attr_idx,
                    struct named_merge_point* named_merge_points)
{
  Lisp_Object inherited_attrs[LFACE_VECTOR_SIZE];
  Lisp_Object attr_val = attrs[attr_idx];

  memcpy(inherited_attrs, attrs, LFACE_VECTOR_SIZE * sizeof(attrs[0]));
  while (UNSPECIFIEDP(attr_val) &&
         !NILP(inherited_attrs[LFACE_INHERIT_INDEX]) &&
         !UNSPECIFIEDP(inherited_attrs[LFACE_INHERIT_INDEX]))
  {
    Lisp_Object parent_face = inherited_attrs[LFACE_INHERIT_INDEX];
    bool ok;

    if (CONSP(parent_face))
    {
      Lisp_Object tail;
      ok = false;
      for (tail = parent_face; !NILP(tail); tail = XCDR(tail))
      {
        ok = get_lface_attributes(w, f, XCAR(tail), inherited_attrs, false,
                                  named_merge_points);
        if (!ok)
          break;
        attr_val = face_inherited_attr(w, f, inherited_attrs, attr_idx,
                                       named_merge_points);
        if (!UNSPECIFIEDP(attr_val))
          break;
      }
      if (!ok) /* bad face? */
        break;
    }
    else
    {
      ok = get_lface_attributes(w, f, parent_face, inherited_attrs, false,
                                named_merge_points);
      if (!ok)
        break;
      attr_val = inherited_attrs[attr_idx];
    }
  }
  return attr_val;
}

/* Chase the chain of inheritance for FACE on frame F, and return
   non-zero if FACE inherits from its CHILD face, directly or
   indirectly.  FACE is either a symbol or a list of face symbols, which
   are two forms of values for the :inherit attribute of a face.  CHILD
   must be a face symbol.  */
static bool face_inheritance_cycle(struct frame* f, Lisp_Object face,
                                   Lisp_Object child)
{
  Lisp_Object face_attrs[LFACE_VECTOR_SIZE];
  Lisp_Object parent_face;
  bool ok, cycle_found = false;

  eassert(SYMBOLP(child));
  if (CONSP(face))
  {
    Lisp_Object tail;
    for (tail = face; CONSP(tail); tail = XCDR(tail))
    {
      Lisp_Object member_face = XCAR(tail);
      ok = get_lface_attributes(NULL, f, member_face, face_attrs, false, NULL);
      if (!ok)
        break;
      parent_face = face_attrs[LFACE_INHERIT_INDEX];
      if (EQ(parent_face, member_face) || EQ(parent_face, child))
        cycle_found = true;
      else if (!NILP(parent_face) && !UNSPECIFIEDP(parent_face) &&
               !IGNORE_DEFFACE_P(parent_face) && !RESET_P(parent_face))
        cycle_found = face_inheritance_cycle(f, parent_face, child);
      if (cycle_found)
        break;
    }
  }
  else if (SYMBOLP(face))
  {
    ok = get_lface_attributes(NULL, f, face, face_attrs, false, NULL);
    if (ok)
    {
      parent_face = face_attrs[LFACE_INHERIT_INDEX];
      if (EQ(parent_face, face) || EQ(parent_face, child))
        cycle_found = true;
      else if (!NILP(parent_face) && !UNSPECIFIEDP(parent_face) &&
               !IGNORE_DEFFACE_P(parent_face) && !RESET_P(parent_face))
        cycle_found = face_inheritance_cycle(f, parent_face, child);
    }
  }

  return cycle_found;
}

/* Merge the named face FACE_NAME on frame F, into the vector of face
   attributes TO.  Use NAMED_MERGE_POINTS to detect loops in face
   inheritance.  Return true if FACE_NAME is a valid face name and
   merging succeeded.  Window W, if non-NULL, is used to filter face
   specifications. */

static bool merge_named_face(struct window* w, struct frame* f,
                             Lisp_Object face_name, Lisp_Object* to,
                             struct named_merge_point* named_merge_points,
                             enum lface_attribute_index attr_filter)
{
  struct named_merge_point named_merge_point;

  if (push_named_merge_point(&named_merge_point, face_name,
                             NAMED_MERGE_POINT_NORMAL, &named_merge_points))
  {
    Lisp_Object from[LFACE_VECTOR_SIZE], val;
    bool ok =
        get_lface_attributes(w, f, face_name, from, false, named_merge_points);
    if (ok && !EQ(face_name, Qdefault))
    {
      struct face* deflt = FACE_FROM_ID(f, DEFAULT_FACE_ID);
      int i;
      for (i = 1; i < LFACE_VECTOR_SIZE; i++)
        if (EQ(from[i], Qreset))
          from[i] = deflt->lface[i];
    }

    if (ok &&
        (attr_filter == 0 /* No filter.  */
         || (!NILP(from[attr_filter]) /* Filter, but specified.  */
             && !UNSPECIFIEDP(from[attr_filter]))
         /* Filter, unspecified, but inherited.  */
         || (!NILP(from[LFACE_INHERIT_INDEX]) &&
             !UNSPECIFIEDP(from[LFACE_INHERIT_INDEX]) &&
             (val = face_inherited_attr(w, f, from, attr_filter,
                                        named_merge_points),
              (!NILP(val) && !UNSPECIFIEDP(val))))))
      merge_face_vectors(w, f, from, to, named_merge_points);

    return ok;
  }
  else
    return false;
}

/* Determine whether the face filter FILTER evaluated in window W
   matches.  W can be NULL if the window context is unknown.

   A face filter is either nil, which always matches, or a list
   (:window PARAMETER VALUE), which matches if the current window has
   a PARAMETER EQ to VALUE.

   This function returns true if the face filter matches, and false if
   it doesn't or if the function encountered an error.  If the filter
   is invalid, set *OK to false and, if ERR_MSGS is true, log an error
   message.  On success, *OK is untouched.  */
static bool evaluate_face_filter(Lisp_Object filter, struct window* w, bool* ok,
                                 bool err_msgs)
{
  Lisp_Object orig_filter = filter;

  /* Inner braces keep compiler happy about the goto skipping variable
     initialization.  */
  {
    if (NILP(filter))
      return true;

    if (face_filters_always_match)
      return true;

    if (!CONSP(filter))
      goto err;

    if (!EQ(XCAR(filter), QCwindow))
      goto err;
    filter = XCDR(filter);

    Lisp_Object parameter = XCAR(filter);
    filter = XCDR(filter);
    if (!CONSP(filter))
      goto err;

    Lisp_Object value = XCAR(filter);
    filter = XCDR(filter);
    if (!NILP(filter))
      goto err;

    if (NILP(Fget(parameter, QCfiltered)))
      Fput(parameter, QCfiltered, Qt);

    bool match = false;
    if (w)
    {
      Lisp_Object found = assq_no_quit(parameter, w->window_parameters);
      if (!NILP(found) && EQ(XCDR(found), value))
        match = true;
    }

    return match;
  }

err:
  if (err_msgs)
    add_to_log("Invalid face filter %S", orig_filter);
  *ok = false;
  return false;
}

/* Determine whether FACE_REF is a "filter" face specification (case
   #4 in merge_face_ref).  If it is, evaluate the filter, and if the
   filter matches, return the filtered face spec.  If the filter does
   not match, return nil.  If FACE_REF is not a filtered face
   specification, return FACE_REF.

   On error, set *OK to false, having logged an error message if
   ERR_MSGS is true, and return nil.  Otherwise, *OK is not touched.

   W is either NULL or a window used to evaluate filters.  If W is
   NULL, no window-based face specification filter matches.
*/
static Lisp_Object filter_face_ref(Lisp_Object face_ref, struct window* w,
                                   bool* ok, bool err_msgs)
{
  Lisp_Object orig_face_ref = face_ref;
  if (!CONSP(face_ref))
    return face_ref;

  /* Inner braces keep compiler happy about the goto skipping variable
     initialization.  */
  {
    if (!EQ(XCAR(face_ref), QCfiltered))
      return face_ref;
    face_ref = XCDR(face_ref);

    if (!CONSP(face_ref))
      goto err;
    Lisp_Object filter = XCAR(face_ref);
    face_ref = XCDR(face_ref);

    if (!CONSP(face_ref))
      goto err;
    Lisp_Object filtered_face_ref = XCAR(face_ref);
    face_ref = XCDR(face_ref);

    if (!NILP(face_ref))
      goto err;

    return evaluate_face_filter(filter, w, ok, err_msgs) ? filtered_face_ref
                                                         : Qnil;
  }

err:
  if (err_msgs)
    add_to_log("Invalid face ref %S", orig_face_ref);
  *ok = false;
  return Qnil;
}

/* Merge face attributes from the lisp `face reference' FACE_REF on
   frame F into the face attribute vector TO as appropriate for
   window W; W is used only for filtering face specs.  If ERR_MSGS
   is non-zero, problems with FACE_REF cause an error message to be
   shown.  Return true if no errors occurred (regardless of the value
   of ERR_MSGS).  Use NAMED_MERGE_POINTS to detect loops in face
   inheritance or list structure; it may be 0 for most callers.

   ATTR_FILTER is the index of a parameter that conditions the merging
   for named faces (case 1) to only the face_ref where
   lface[merge_face_ref] is non-nil.  To merge unconditionally set this
   value to 0.

   FACE_REF may be a single face specification or a list of such
   specifications.  Each face specification can be:

   1. A symbol or string naming a Lisp face.

   2. A property list of the form (KEYWORD VALUE ...) where each
   KEYWORD is a face attribute name, and value is an appropriate value
   for that attribute.

   3. Conses or the form (FOREGROUND-COLOR . COLOR) or
   (BACKGROUND-COLOR . COLOR) where COLOR is a color name.  This is
   for compatibility with 20.2.

   4. Conses of the form
   (:filtered (:window PARAMETER VALUE) FACE-SPECIFICATION),
   which applies FACE-SPECIFICATION only if the given face attributes
   are being evaluated in the context of a window with a parameter
   named PARAMETER being EQ VALUE.  In this case, W specifies the window
   for which the filtered face spec is to be evaluated.

   5. nil, which means to merge nothing.

   Face specifications earlier in lists take precedence over later
   specifications.  */

static bool merge_face_ref(struct window* w, struct frame* f,
                           Lisp_Object face_ref, Lisp_Object* to, bool err_msgs,
                           struct named_merge_point* named_merge_points,
                           enum lface_attribute_index attr_filter)
{
  bool ok = true; /* Succeed without an error? */
  Lisp_Object filtered_face_ref;
  bool attr_filter_passed = false;

  filtered_face_ref = face_ref;
  do
  {
    face_ref = filtered_face_ref;
    filtered_face_ref = filter_face_ref(face_ref, w, &ok, err_msgs);
  }
  while (ok && !EQ(face_ref, filtered_face_ref));

  if (!ok)
    return false;

  if (NILP(face_ref))
    return true;

  if (CONSP(face_ref))
  {
    Lisp_Object first = XCAR(face_ref);

    if (EQ(first, Qforeground_color) || EQ(first, Qbackground_color))
    {
      /* One of (FOREGROUND-COLOR . COLOR) or (BACKGROUND-COLOR
         . COLOR).  COLOR must be a string.  */
      Lisp_Object color_name = XCDR(face_ref);
      Lisp_Object color = first;

      if (STRINGP(color_name))
      {
        if (EQ(color, Qforeground_color))
          to[LFACE_FOREGROUND_INDEX] = color_name;
        else
          to[LFACE_BACKGROUND_INDEX] = color_name;
      }
      else
      {
        if (err_msgs)
          add_to_log("Invalid face color %S", color_name);
        ok = false;
      }
    }
    else if (SYMBOLP(first) && *SDATA(SYMBOL_NAME(first)) == ':')
    {
      /* Assume this is the property list form.  */
      if (attr_filter > 0)
      {
        eassert(attr_filter < LFACE_VECTOR_SIZE);
        /* ATTR_FILTER positive means don't merge this face if
     the corresponding attribute is nil, or not mentioned,
     or if it's unspecified and the face doesn't inherit
     from a face whose attribute is non-nil.  The code
     below determines whether a face given as a property
     list shall be merged.  */
        Lisp_Object parent_face = Qnil;
        bool attr_filter_seen = false;
        Lisp_Object face_ref_tem = face_ref;
        while (CONSP(face_ref_tem) && CONSP(XCDR(face_ref_tem)))
        {
          Lisp_Object keyword = XCAR(face_ref_tem);
          Lisp_Object value = XCAR(XCDR(face_ref_tem));

          if (EQ(keyword, face_attr_sym[attr_filter]))
          {
            attr_filter_seen = true;
            if (NILP(value))
              return true;
          }
          else if (EQ(keyword, QCinherit))
            parent_face = value;
          face_ref_tem = XCDR(XCDR(face_ref_tem));
        }
        if (!attr_filter_seen)
        {
          if (NILP(parent_face))
            return true;

          Lisp_Object scratch_attrs[LFACE_VECTOR_SIZE];
          int i;

          scratch_attrs[0] = Qface;
          for (i = 1; i < LFACE_VECTOR_SIZE; i++)
            scratch_attrs[i] = Qunspecified;
          if (!merge_face_ref(w, f, parent_face, scratch_attrs, err_msgs,
                              named_merge_points, 0))
          {
            add_to_log("Invalid face attribute %S %S", QCinherit, parent_face);
            return false;
          }
          if (NILP(scratch_attrs[attr_filter]) ||
              UNSPECIFIEDP(scratch_attrs[attr_filter]))
            return true;
        }
        attr_filter_passed = true;
      }
      while (CONSP(face_ref) && CONSP(XCDR(face_ref)))
      {
        Lisp_Object keyword = XCAR(face_ref);
        Lisp_Object value = XCAR(XCDR(face_ref));
        bool err = false;

        /* Specifying `unspecified' is a no-op.  */
        if (EQ(value, Qunspecified))
          ;
        else if (EQ(keyword, QCfamily))
        {
          if (STRINGP(value))
          {
            to[LFACE_FAMILY_INDEX] = value;
            font_clear_prop(to, FONT_FAMILY_INDEX);
          }
          else
            err = true;
        }
        else if (EQ(keyword, QCfoundry))
        {
          if (STRINGP(value))
          {
            to[LFACE_FOUNDRY_INDEX] = value;
            font_clear_prop(to, FONT_FOUNDRY_INDEX);
          }
          else
            err = true;
        }
        else if (EQ(keyword, QCheight))
        {
          Lisp_Object new_height =
              merge_face_heights(value, to[LFACE_HEIGHT_INDEX], Qnil);

          if (!NILP(new_height))
          {
            to[LFACE_HEIGHT_INDEX] = new_height;
            font_clear_prop(to, FONT_SIZE_INDEX);
          }
          else
            err = true;
        }
        else if (EQ(keyword, QCweight))
        {
          if (SYMBOLP(value) && FONT_WEIGHT_NAME_NUMERIC(value) >= 0)
          {
            to[LFACE_WEIGHT_INDEX] = value;
            font_clear_prop(to, FONT_WEIGHT_INDEX);
          }
          else
            err = true;
        }
        else if (EQ(keyword, QCslant))
        {
          if (SYMBOLP(value) && FONT_SLANT_NAME_NUMERIC(value) >= 0)
          {
            to[LFACE_SLANT_INDEX] = value;
            font_clear_prop(to, FONT_SLANT_INDEX);
          }
          else
            err = true;
        }
        else if (EQ(keyword, QCunderline))
        {
          if (EQ(value, Qt) || NILP(value) || STRINGP(value) || CONSP(value))
            to[LFACE_UNDERLINE_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCoverline))
        {
          if (EQ(value, Qt) || NILP(value) || STRINGP(value))
            to[LFACE_OVERLINE_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCstrike_through))
        {
          if (EQ(value, Qt) || NILP(value) || STRINGP(value))
            to[LFACE_STRIKE_THROUGH_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCbox))
        {
          if (EQ(value, Qt))
            value = make_fixnum(1);
          if ((FIXNUMP(value) && XFIXNUM(value) != 0) || STRINGP(value) ||
              CONSP(value) || NILP(value))
            to[LFACE_BOX_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCinverse_video))
        {
          if (EQ(value, Qt) || NILP(value))
            to[LFACE_INVERSE_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCforeground))
        {
          if (STRINGP(value))
            to[LFACE_FOREGROUND_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCdistant_foreground))
        {
          if (STRINGP(value))
            to[LFACE_DISTANT_FOREGROUND_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCbackground))
        {
          if (STRINGP(value))
            to[LFACE_BACKGROUND_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCwidth))
        {
          if (SYMBOLP(value) && FONT_WIDTH_NAME_NUMERIC(value) >= 0)
          {
            to[LFACE_SWIDTH_INDEX] = value;
            font_clear_prop(to, FONT_WIDTH_INDEX);
          }
          else
            err = true;
        }
        else if (EQ(keyword, QCfont))
        {
          if (FONTP(value))
            to[LFACE_FONT_INDEX] = value;
          else
            err = true;
        }
        else if (EQ(keyword, QCinherit))
        {
          /* This is not really very useful; it's just like a
             normal face reference.  */
          if (attr_filter_passed)
          {
            /* We already know that this face was tested
         against attr_filter and was found applicable,
         so don't pass attr_filter to merge_face_ref.
         This is for when a face is specified like
         (:inherit FACE :extend t), but the parent
         FACE itself doesn't specify :extend.  */
            if (!merge_face_ref(w, f, value, to, err_msgs, named_merge_points,
                                0))
              err = true;
          }
          else if (!merge_face_ref(w, f, value, to, err_msgs,
                                   named_merge_points, attr_filter))
            err = true;
        }
        else if (EQ(keyword, QCextend))
        {
          if (EQ(value, Qt) || NILP(value))
            to[LFACE_EXTEND_INDEX] = value;
          else
            err = true;
        }
        else
          err = true;

        if (err)
        {
          add_to_log("Invalid face attribute %S %S", keyword, value);
          ok = false;
        }

        face_ref = XCDR(XCDR(face_ref));
      }
    }
    else
    {
      /* This is a list of face refs.  Those at the beginning of the
         list take precedence over what follows, so we have to merge
         from the end backwards.  */
      Lisp_Object next = XCDR(face_ref);

      if (!NILP(next))
        ok = merge_face_ref(w, f, next, to, err_msgs, named_merge_points,
                            attr_filter);

      if (!merge_face_ref(w, f, first, to, err_msgs, named_merge_points,
                          attr_filter))
        ok = false;
    }
  }
  else
  {
    /* FACE_REF ought to be a face name.  */
    ok = merge_named_face(w, f, face_ref, to, named_merge_points, attr_filter);
    if (!ok && err_msgs)
      add_to_log("Invalid face reference: %s", face_ref);
  }

  return ok;
}


DEFUN ("internal-make-lisp-face", Finternal_make_lisp_face,
       Sinternal_make_lisp_face, 1, 2, 0,
       doc: /* Make FACE, a symbol, a Lisp face with all attributes nil.
If FACE was not known as a face before, create a new one.
If optional argument FRAME is specified, make a frame-local face
for that frame.  Otherwise operate on the global face definition.
Value is a vector of face attributes.  */)
(Lisp_Object face, Lisp_Object frame)
{
  Lisp_Object global_lface, lface;
  struct frame* f;
  int i;

  CHECK_SYMBOL(face);
  global_lface = lface_from_face_name(NULL, face, false);

  if (!NILP(frame))
  {
    CHECK_LIVE_FRAME(frame);
    f = XFRAME(frame);
    lface = lface_from_face_name(f, face, false);
  }
  else
    f = NULL, lface = Qnil;

  /* Add a global definition if there is none.  */
  if (NILP(global_lface))
  {
    /* Assign the new Lisp face a unique ID.  The mapping from Lisp
 face id to Lisp face is given by the vector lface_id_to_name.
 The mapping from Lisp face to Lisp face id is given by the
 property `face' of the Lisp face name.  */
    if (next_lface_id == lface_id_to_name_size)
      lface_id_to_name = xpalloc(lface_id_to_name, &lface_id_to_name_size, 1,
                                 MAX_FACE_ID, sizeof *lface_id_to_name);

    Lisp_Object face_id = make_fixnum(next_lface_id);
    lface_id_to_name[next_lface_id] = face;
    Fput(face, Qface, face_id);
    ++next_lface_id;

    global_lface = make_vector(LFACE_VECTOR_SIZE, Qunspecified);
    ASET(global_lface, 0, Qface);
    Fputhash(face, Fcons(face_id, global_lface), Vface_new_frame_defaults);
  }
  else if (f == NULL)
    for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
      ASET(global_lface, i, Qunspecified);

  /* Add a frame-local definition.  */
  if (f)
  {
    if (NILP(lface))
    {
      lface = make_vector(LFACE_VECTOR_SIZE, Qunspecified);
      ASET(lface, 0, Qface);
      Fputhash(face, lface, f->face_hash_table);
    }
    else
      for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
        ASET(lface, i, Qunspecified);
  }
  else
    lface = global_lface;

  /* Changing a named face means that all realized faces depending on
     that face are invalid.  Since we cannot tell which realized faces
     depend on the face, make sure they are all removed.  This is done
     by setting face_change.  The next call to init_iterator will then
     free realized faces.  */
  if (NILP(Fget(face, Qface_no_inherit)))
  {
    if (f)
    {
      f->face_change = true;
      fset_redisplay(f);
    }
    else
    {
      face_change = true;
      windows_or_buffers_changed = 54;
    }
  }

  eassert(LFACEP(lface));
  check_lface(lface);
  return lface;
}


DEFUN ("internal-lisp-face-p", Finternal_lisp_face_p,
       Sinternal_lisp_face_p, 1, 2, 0,
       doc: /* Return non-nil if FACE names a face.
FACE should be a symbol or string.
If optional second argument FRAME is non-nil, check for the
existence of a frame-local face with name FACE on that frame.
Otherwise check for the existence of a global face.  */)
(Lisp_Object face, Lisp_Object frame)
{
  Lisp_Object lface;

  face = resolve_face_name(face, true);

  if (!NILP(frame))
  {
    CHECK_LIVE_FRAME(frame);
    lface = lface_from_face_name(XFRAME(frame), face, false);
  }
  else
    lface = lface_from_face_name(NULL, face, false);

  return lface;
}


DEFUN ("internal-copy-lisp-face", Finternal_copy_lisp_face,
       Sinternal_copy_lisp_face, 4, 4, 0,
       doc: /* Copy face FROM to TO.
If FRAME is t, copy the global face definition of FROM.
Otherwise, copy the frame-local definition of FROM on FRAME.
If NEW-FRAME is a frame, copy that data into the frame-local
definition of TO on NEW-FRAME.  If NEW-FRAME is nil,
FRAME controls where the data is copied to.

The value is TO.  */)
(Lisp_Object from, Lisp_Object to, Lisp_Object frame, Lisp_Object new_frame)
{
  Lisp_Object lface, copy;
  struct frame* f;

  CHECK_SYMBOL(from);
  CHECK_SYMBOL(to);

  if (EQ(frame, Qt))
  {
    /* Copy global definition of FROM.  We don't make copies of
 strings etc. because 20.2 didn't do it either.  */
    lface = lface_from_face_name(NULL, from, true);
    copy = Finternal_make_lisp_face(to, Qnil);
    f = NULL;
  }
  else
  {
    /* Copy frame-local definition of FROM.  */
    if (NILP(new_frame))
      new_frame = frame;
    CHECK_LIVE_FRAME(frame);
    CHECK_LIVE_FRAME(new_frame);
    lface = lface_from_face_name(XFRAME(frame), from, true);
    copy = Finternal_make_lisp_face(to, new_frame);
    f = XFRAME(new_frame);
  }

  vcopy(copy, 0, xvector_contents(lface), LFACE_VECTOR_SIZE);

  /* Changing a named face means that all realized faces depending on
     that face are invalid.  Since we cannot tell which realized faces
     depend on the face, make sure they are all removed.  This is done
     by setting face_change.  The next call to init_iterator will then
     free realized faces.  */
  if (NILP(Fget(to, Qface_no_inherit)))
  {
    if (f)
    {
      f->face_change = true;
      fset_redisplay(f);
    }
    else
    {
      face_change = true;
      windows_or_buffers_changed = 55;
    }
  }

  return to;
}


#define HANDLE_INVALID_NIL_VALUE(A, F)                                         \
  if (NILP(value))                                                             \
  {                                                                            \
    add_to_log("Warning: setting attribute `%s' of face `%s': nil "            \
               "value is invalid, use `unspecified' instead.",                 \
               A, F);                                                          \
    /* Compatibility with 20.x.  */                                            \
    value = Qunspecified;                                                      \
  }

DEFUN ("internal-set-lisp-face-attribute", Finternal_set_lisp_face_attribute,
       Sinternal_set_lisp_face_attribute, 3, 4, 0,
       doc: /* Set attribute ATTR of FACE to VALUE.
FRAME being a frame means change the face on that frame.
FRAME nil means change the face of the selected frame.
FRAME t means change the default for new frames.
FRAME 0 means change the face on all frames, and change the default
  for new frames.  */)
(Lisp_Object face, Lisp_Object attr, Lisp_Object value, Lisp_Object frame)
{
  Lisp_Object lface;
  Lisp_Object old_value = Qnil;
  /* Set one of enum font_property_index (> 0) if ATTR is one of
     font-related attributes other than QCfont and QCfontset.  */
  enum font_property_index prop_index = 0;
  struct frame* f;

  CHECK_SYMBOL(face);
  CHECK_SYMBOL(attr);

  face = resolve_face_name(face, true);

  /* If FRAME is 0, change face on all frames, and change the
     default for new frames.  */
  if (FIXNUMP(frame) && XFIXNUM(frame) == 0)
  {
    Lisp_Object tail;
    Finternal_set_lisp_face_attribute(face, attr, value, Qt);
    FOR_EACH_FRAME(tail, frame)
    Finternal_set_lisp_face_attribute(face, attr, value, frame);
    return face;
  }

  /* Set lface to the Lisp attribute vector of FACE.  */
  if (EQ(frame, Qt))
  {
    f = NULL;
    lface = lface_from_face_name(NULL, face, true);

    /* When updating face--new-frame-defaults, we put :ignore-defface
 where the caller wants `unspecified'.  This forces the frame
 defaults to ignore the defface value.  Otherwise, the defface
 will take effect, which is generally not what is intended.
 The value of that attribute will be inherited from some other
 face during face merging.  See internal_merge_in_global_face. */
    if (UNSPECIFIEDP(value))
      value = QCignore_defface;
  }
  else
  {
    if (NILP(frame))
      frame = selected_frame;

    CHECK_LIVE_FRAME(frame);
    f = XFRAME(frame);

    lface = lface_from_face_name(f, face, false);

    /* If a frame-local face doesn't exist yet, create one.  */
    if (NILP(lface))
      lface = Finternal_make_lisp_face(face, frame);
  }

  if (EQ(attr, QCfamily))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_STRING(value);
      if (SCHARS(value) == 0)
        signal_error("Invalid face family", value);
    }
    old_value = LFACE_FAMILY(lface);
    ASET(lface, LFACE_FAMILY_INDEX, value);
    prop_index = FONT_FAMILY_INDEX;
  }
  else if (EQ(attr, QCfoundry))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_STRING(value);
      if (SCHARS(value) == 0)
        signal_error("Invalid face foundry", value);
    }
    old_value = LFACE_FOUNDRY(lface);
    ASET(lface, LFACE_FOUNDRY_INDEX, value);
    prop_index = FONT_FOUNDRY_INDEX;
  }
  else if (EQ(attr, QCheight))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      if (EQ(face, Qdefault))
      {
        /* The default face must have an absolute size.  */
        if (!FIXNUMP(value) || XFIXNUM(value) <= 0)
          signal_error("Default face height not absolute and positive", value);
      }
      else
      {
        /* For non-default faces, do a test merge with a random
     height to see if VALUE's ok. */
        Lisp_Object test = merge_face_heights(value, make_fixnum(10), Qnil);
        if (!FIXNUMP(test) || XFIXNUM(test) <= 0)
          signal_error("Face height does not produce a positive integer",
                       value);
      }
    }

    old_value = LFACE_HEIGHT(lface);
    ASET(lface, LFACE_HEIGHT_INDEX, value);
    prop_index = FONT_SIZE_INDEX;
  }
  else if (EQ(attr, QCweight))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_SYMBOL(value);
      if (FONT_WEIGHT_NAME_NUMERIC(value) < 0)
        signal_error("Invalid face weight", value);
    }
    old_value = LFACE_WEIGHT(lface);
    ASET(lface, LFACE_WEIGHT_INDEX, value);
    prop_index = FONT_WEIGHT_INDEX;
  }
  else if (EQ(attr, QCslant))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_SYMBOL(value);
      if (FONT_SLANT_NAME_NUMERIC(value) < 0)
        signal_error("Invalid face slant", value);
    }
    old_value = LFACE_SLANT(lface);
    ASET(lface, LFACE_SLANT_INDEX, value);
    prop_index = FONT_SLANT_INDEX;
  }
  else if (EQ(attr, QCunderline))
  {
    bool valid_p = false;

    if (UNSPECIFIEDP(value) || IGNORE_DEFFACE_P(value) || RESET_P(value))
      valid_p = true;
    else if (NILP(value) || EQ(value, Qt))
      valid_p = true;
    else if (STRINGP(value) && SCHARS(value) > 0)
      valid_p = true;
    else if (CONSP(value))
    {
      Lisp_Object key, val, list;

      list = value;
      /* FIXME?  This errs on the side of acceptance.  Eg it accepts:
           (defface foo '((t :underline 'foo) "doc")
         Maybe this is intentional, maybe it isn't.
         Non-nil symbols other than t are not documented as being valid.
         Eg compare with inverse-video, which explicitly rejects them.
      */
      valid_p = true;

      while (!NILP(CAR_SAFE(list)))
      {
        key = CAR_SAFE(list);
        list = CDR_SAFE(list);
        val = CAR_SAFE(list);
        list = CDR_SAFE(list);

        if (NILP(key) || (NILP(val) && !EQ(key, QCposition)))
        {
          valid_p = false;
          break;
        }

        else if (EQ(key, QCcolor) &&
                 !(EQ(val, Qforeground_color) ||
                   (STRINGP(val) && SCHARS(val) > 0)))
        {
          valid_p = false;
          break;
        }

        else if (EQ(key, QCstyle) &&
                 !(EQ(val, Qline) || EQ(val, Qdouble_line) || EQ(val, Qwave) ||
                   EQ(val, Qdots) || EQ(val, Qdashes)))
        {
          valid_p = false;
          break;
        }
      }
    }

    if (!valid_p)
      signal_error("Invalid face underline", value);

    old_value = LFACE_UNDERLINE(lface);
    ASET(lface, LFACE_UNDERLINE_INDEX, value);
  }
  else if (EQ(attr, QCoverline))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
      if ((SYMBOLP(value) && !EQ(value, Qt) && !NILP(value))
          /* Overline color.  */
          || (STRINGP(value) && SCHARS(value) == 0))
        signal_error("Invalid face overline", value);

    old_value = LFACE_OVERLINE(lface);
    ASET(lface, LFACE_OVERLINE_INDEX, value);
  }
  else if (EQ(attr, QCstrike_through))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
      if ((SYMBOLP(value) && !EQ(value, Qt) && !NILP(value))
          /* Strike-through color.  */
          || (STRINGP(value) && SCHARS(value) == 0))
        signal_error("Invalid face strike-through", value);

    old_value = LFACE_STRIKE_THROUGH(lface);
    ASET(lface, LFACE_STRIKE_THROUGH_INDEX, value);
  }
  else if (EQ(attr, QCbox))
  {
    bool valid_p;

    /* Allow t meaning a simple box of width 1 in foreground color
 of the face.  */
    if (EQ(value, Qt))
      value = make_fixnum(1);

    if (UNSPECIFIEDP(value) || IGNORE_DEFFACE_P(value) || RESET_P(value))
      valid_p = true;
    else if (NILP(value))
      valid_p = true;
    else if (FIXNUMP(value))
      valid_p = XFIXNUM(value) != 0;
    else if (STRINGP(value))
      valid_p = SCHARS(value) > 0;
    else if (CONSP(value) && FIXNUMP(XCAR(value)) && FIXNUMP(XCDR(value)))
      valid_p = true;
    else if (CONSP(value))
    {
      Lisp_Object tem;

      tem = value;
      while (CONSP(tem))
      {
        Lisp_Object k, v;

        k = XCAR(tem);
        tem = XCDR(tem);
        if (!CONSP(tem))
          break;
        v = XCAR(tem);

        if (EQ(k, QCline_width))
        {
          if ((!CONSP(v) || !FIXNUMP(XCAR(v)) || XFIXNUM(XCAR(v)) == 0 ||
               !FIXNUMP(XCDR(v)) || XFIXNUM(XCDR(v)) == 0) &&
              (!FIXNUMP(v) || XFIXNUM(v) == 0))
            break;
        }
        else if (EQ(k, QCcolor))
        {
          if (!NILP(v) && (!STRINGP(v) || SCHARS(v) == 0))
            break;
        }
        else if (EQ(k, QCstyle))
        {
          if (!NILP(v) && !EQ(v, Qpressed_button) && !EQ(v, Qreleased_button) &&
              !EQ(v, Qflat_button))
            break;
        }
        else
          break;

        tem = XCDR(tem);
      }

      valid_p = NILP(tem);
    }
    else
      valid_p = false;

    if (!valid_p)
      signal_error("Invalid face box", value);

    old_value = LFACE_BOX(lface);
    ASET(lface, LFACE_BOX_INDEX, value);
  }
  else if (EQ(attr, QCinverse_video))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_SYMBOL(value);
      if (!EQ(value, Qt) && !NILP(value))
        signal_error("Invalid inverse-video face attribute value", value);
    }
    old_value = LFACE_INVERSE(lface);
    ASET(lface, LFACE_INVERSE_INDEX, value);
  }
  else if (EQ(attr, QCextend))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_SYMBOL(value);
      if (!EQ(value, Qt) && !NILP(value))
        signal_error("Invalid extend face attribute value", value);
    }
    old_value = LFACE_EXTEND(lface);
    ASET(lface, LFACE_EXTEND_INDEX, value);
  }
  else if (EQ(attr, QCforeground))
  {
    HANDLE_INVALID_NIL_VALUE(QCforeground, face);
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      /* Don't check for valid color names here because it depends
         on the frame (display) whether the color will be valid
         when the face is realized.  */
      CHECK_STRING(value);
      if (SCHARS(value) == 0)
        signal_error("Empty foreground color value", value);
    }
    old_value = LFACE_FOREGROUND(lface);
    ASET(lface, LFACE_FOREGROUND_INDEX, value);
  }
  else if (EQ(attr, QCdistant_foreground))
  {
    HANDLE_INVALID_NIL_VALUE(QCdistant_foreground, face);
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      /* Don't check for valid color names here because it depends
         on the frame (display) whether the color will be valid
         when the face is realized.  */
      CHECK_STRING(value);
      if (SCHARS(value) == 0)
        signal_error("Empty distant-foreground color value", value);
    }
    old_value = LFACE_DISTANT_FOREGROUND(lface);
    ASET(lface, LFACE_DISTANT_FOREGROUND_INDEX, value);
  }
  else if (EQ(attr, QCbackground))
  {
    HANDLE_INVALID_NIL_VALUE(QCbackground, face);
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      /* Don't check for valid color names here because it depends
         on the frame (display) whether the color will be valid
         when the face is realized.  */
      CHECK_STRING(value);
      if (SCHARS(value) == 0)
        signal_error("Empty background color value", value);
    }
    old_value = LFACE_BACKGROUND(lface);
    ASET(lface, LFACE_BACKGROUND_INDEX, value);
  }
  else if (EQ(attr, QCwidth))
  {
    if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) && !RESET_P(value))
    {
      CHECK_SYMBOL(value);
      if (FONT_WIDTH_NAME_NUMERIC(value) < 0)
        signal_error("Invalid face width", value);
    }
    old_value = LFACE_SWIDTH(lface);
    ASET(lface, LFACE_SWIDTH_INDEX, value);
    prop_index = FONT_WIDTH_INDEX;
  }
  else if (EQ(attr, QCinherit))
  {
    Lisp_Object tail;
    if (SYMBOLP(value))
      tail = Qnil;
    else
      for (tail = value; CONSP(tail); tail = XCDR(tail))
        if (!SYMBOLP(XCAR(tail)))
          break;
    if (EQ(value, face) || face_inheritance_cycle(f, value, face))
      signal_error("Face inheritance results in inheritance cycle", value);
    else if (NILP(tail))
      ASET(lface, LFACE_INHERIT_INDEX, value);
    else
      signal_error("Invalid face inheritance", value);
  }
  else if (EQ(attr, QCbold))
  {
    old_value = LFACE_WEIGHT(lface);
    if (RESET_P(value))
      ASET(lface, LFACE_WEIGHT_INDEX, value);
    else
      ASET(lface, LFACE_WEIGHT_INDEX, NILP(value) ? Qnormal : Qbold);
    prop_index = FONT_WEIGHT_INDEX;
  }
  else if (EQ(attr, QCitalic))
  {
    attr = QCslant;
    old_value = LFACE_SLANT(lface);
    if (RESET_P(value))
      ASET(lface, LFACE_SLANT_INDEX, value);
    else
      ASET(lface, LFACE_SLANT_INDEX, NILP(value) ? Qnormal : Qitalic);
    prop_index = FONT_SLANT_INDEX;
  }
  else
    signal_error("Invalid face attribute name", attr);

  if (prop_index)
  {
    /* If a font-related attribute other than QCfont and QCfontset
 is specified, and if the original QCfont attribute has a font
 (font-spec or font-object), set the corresponding property in
 the font to nil so that the font selector doesn't think that
 the attribute is mandatory.  Also, clear the average
 width.  */
    font_clear_prop(XVECTOR(lface)->contents, prop_index);
  }

  /* Changing a named face means that all realized faces depending on
     that face are invalid.  Since we cannot tell which realized faces
     depend on the face, make sure they are all removed.  This is done
     by setting face_change.  The next call to init_iterator will then
     free realized faces.  */
  if (!EQ(frame, Qt) && NILP(Fget(face, Qface_no_inherit)) &&
      NILP(Fequal(old_value, value)))
  {
    f->face_change = true;
    fset_redisplay(f);
  }

  if (!UNSPECIFIEDP(value) && !IGNORE_DEFFACE_P(value) &&
      NILP(Fequal(old_value, value)))
  {
    Lisp_Object param;

    param = Qnil;

    if (EQ(face, Qdefault))
    {
      if (EQ(attr, QCforeground))
        param = Qforeground_color;
      else if (EQ(attr, QCbackground))
        param = Qbackground_color;
    }
    else if (EQ(face, Qmenu))
    {
      /* Indicate that we have to update the menu bar when realizing
         faces on FRAME.  FRAME t change the default for new frames.
         We do this by setting the flag in new face caches.  */
      if (FRAMEP(frame))
      {
        struct frame* f = XFRAME(frame);
        if (FRAME_FACE_CACHE(f) == NULL)
          FRAME_FACE_CACHE(f) = make_face_cache(f);
        FRAME_FACE_CACHE(f)->menu_face_changed_p = true;
      }
      else
        menu_face_changed_default = true;
    }

    if (!NILP(param))
    {
      if (EQ(frame, Qt))
      /* Update `default-frame-alist', which is used for new frames.  */
      {
        store_in_alist(&Vdefault_frame_alist, param, value);
      }
      else
      /* Update the current frame's parameters.  */
      {
        AUTO_FRAME_ARG(arg, param, value);
        Fmodify_frame_parameters(frame, arg);
      }
    }
  }

  return face;
}


/* Update the corresponding face when frame parameter PARAM on frame F
   has been assigned the value NEW_VALUE.  */

void update_face_from_frame_parameter(struct frame* f, Lisp_Object param,
                                      Lisp_Object new_value)
{
  Lisp_Object face = Qnil;
  Lisp_Object lface;

  /* If there are no faces yet, give up.  This is the case when called
     from Fx_create_frame, and we do the necessary things later in
     face-set-after-frame-defaults.  */
  if (XFIXNAT(Fhash_table_count(f->face_hash_table)) == 0)
    return;

  if (EQ(param, Qforeground_color))
  {
    face = Qdefault;
    lface = lface_from_face_name(f, face, true);
    ASET(lface, LFACE_FOREGROUND_INDEX,
         (STRINGP(new_value) ? new_value : Qunspecified));
    realize_basic_faces(f);
  }
  else if (EQ(param, Qbackground_color))
  {
    Lisp_Object frame;

    /* Changing the background color might change the background
 mode, so that we have to load new defface specs.
 Call frame-set-background-mode to do that.  */
    XSETFRAME(frame, f);
    calln(Qframe_set_background_mode, frame);

    face = Qdefault;
    lface = lface_from_face_name(f, face, true);
    ASET(lface, LFACE_BACKGROUND_INDEX,
         (STRINGP(new_value) ? new_value : Qunspecified));
    realize_basic_faces(f);
  }

  /* Changing a named face means that all realized faces depending on
     that face are invalid.  Since we cannot tell which realized faces
     depend on the face, make sure they are all removed.  This is done
     by setting face_change.  The next call to init_iterator will then
     free realized faces.  */
  if (!NILP(face) && NILP(Fget(face, Qface_no_inherit)))
  {
    f->face_change = true;
    fset_redisplay(f);
  }
}

/***********************************************************************
            Menu face
 ***********************************************************************/

DEFUN ("face-attribute-relative-p", Fface_attribute_relative_p,
       Sface_attribute_relative_p,
       2, 2, 0,
       doc: /* Check whether a face attribute value is relative.
Specifically, this function returns t if the attribute ATTRIBUTE
with the value VALUE is relative.

A relative value is one that doesn't entirely override whatever is
inherited from another face.  For most possible attributes,
the only relative value that users see is `unspecified'.
However, for :height, floating point values are also relative.  */)
(Lisp_Object attribute, Lisp_Object value)
{
  if (EQ(value, Qunspecified) || (EQ(value, QCignore_defface)))
    return Qt;
  else if (EQ(attribute, QCheight))
    return FIXNUMP(value) ? Qnil : Qt;
  else
    return Qnil;
}

DEFUN ("merge-face-attribute", Fmerge_face_attribute, Smerge_face_attribute,
       3, 3, 0,
       doc: /* Return face ATTRIBUTE VALUE1 merged with VALUE2.
If VALUE1 or VALUE2 are absolute (see `face-attribute-relative-p'), then
the result will be absolute, otherwise it will be relative.  */)
(Lisp_Object attribute, Lisp_Object value1, Lisp_Object value2)
{
  if (EQ(value1, Qunspecified) || EQ(value1, QCignore_defface))
    return value2;
  else if (EQ(attribute, QCheight))
    return merge_face_heights(value1, value2, value1);
  else
    return value1;
}


DEFUN ("internal-get-lisp-face-attribute", Finternal_get_lisp_face_attribute,
       Sinternal_get_lisp_face_attribute,
       2, 3, 0,
       doc: /* Return face attribute KEYWORD of face SYMBOL.
If SYMBOL does not name a valid Lisp face or KEYWORD isn't a valid
face attribute name, signal an error.
If the optional argument FRAME is given, report on face SYMBOL in that
frame.  If FRAME is t, report on the defaults for face SYMBOL (for new
frames).  If FRAME is omitted or nil, use the selected frame.  */)
(Lisp_Object symbol, Lisp_Object keyword, Lisp_Object frame)
{
  struct frame* f = EQ(frame, Qt) ? NULL : decode_live_frame(frame);
  Lisp_Object lface = lface_from_face_name(f, symbol, true), value = Qnil;

  CHECK_SYMBOL(symbol);
  CHECK_SYMBOL(keyword);

  if (EQ(keyword, QCfamily))
    value = LFACE_FAMILY(lface);
  else if (EQ(keyword, QCfoundry))
    value = LFACE_FOUNDRY(lface);
  else if (EQ(keyword, QCheight))
    value = LFACE_HEIGHT(lface);
  else if (EQ(keyword, QCweight))
    value = LFACE_WEIGHT(lface);
  else if (EQ(keyword, QCslant))
    value = LFACE_SLANT(lface);
  else if (EQ(keyword, QCunderline))
    value = LFACE_UNDERLINE(lface);
  else if (EQ(keyword, QCoverline))
    value = LFACE_OVERLINE(lface);
  else if (EQ(keyword, QCstrike_through))
    value = LFACE_STRIKE_THROUGH(lface);
  else if (EQ(keyword, QCbox))
    value = LFACE_BOX(lface);
  else if (EQ(keyword, QCinverse_video))
    value = LFACE_INVERSE(lface);
  else if (EQ(keyword, QCforeground))
    value = LFACE_FOREGROUND(lface);
  else if (EQ(keyword, QCdistant_foreground))
    value = LFACE_DISTANT_FOREGROUND(lface);
  else if (EQ(keyword, QCbackground))
    value = LFACE_BACKGROUND(lface);
  else if (EQ(keyword, QCstipple))
    value = LFACE_STIPPLE(lface);
  else if (EQ(keyword, QCwidth))
    value = LFACE_SWIDTH(lface);
  else if (EQ(keyword, QCinherit))
    value = LFACE_INHERIT(lface);
  else if (EQ(keyword, QCextend))
    value = LFACE_EXTEND(lface);
  else if (EQ(keyword, QCfont))
    value = LFACE_FONT(lface);
  else if (EQ(keyword, QCfontset))
    value = LFACE_FONTSET(lface);
  else
    signal_error("Invalid face attribute name", keyword);

  if (IGNORE_DEFFACE_P(value))
    return Qunspecified;

  return value;
}


DEFUN ("internal-lisp-face-attribute-values",
       Finternal_lisp_face_attribute_values,
       Sinternal_lisp_face_attribute_values, 1, 1, 0,
       doc: /* Return a list of valid discrete values for face attribute ATTR.
Value is nil if ATTR doesn't have a discrete set of valid values.  */)
(Lisp_Object attr)
{
  Lisp_Object result = Qnil;

  CHECK_SYMBOL(attr);

  if (EQ(attr, QCunderline) || EQ(attr, QCoverline) ||
      EQ(attr, QCstrike_through) || EQ(attr, QCinverse_video) ||
      EQ(attr, QCextend))
    result = list2(Qt, Qnil);

  return result;
}


DEFUN ("internal-merge-in-global-face", Finternal_merge_in_global_face,
       Sinternal_merge_in_global_face, 2, 2, 0,
       doc: /* Add attributes from frame-default definition of FACE to FACE on FRAME.
Default face attributes override any local face attributes.  */)
(Lisp_Object face, Lisp_Object frame)
{
  int i;
  Lisp_Object global_lface, local_lface, *gvec, *lvec;
  struct frame* f = XFRAME(frame);

  CHECK_LIVE_FRAME(frame);
  global_lface = lface_from_face_name(NULL, face, true);
  local_lface = lface_from_face_name(f, face, false);
  if (NILP(local_lface))
    local_lface = Finternal_make_lisp_face(face, frame);

  /* Make every specified global attribute override the local one.
     BEWARE!! This is only used from `face-set-after-frame-default' where
     the local frame is defined from default specs in `face-defface-spec'
     and those should be overridden by global settings.  Hence the strange
     "global before local" priority.  */
  lvec = XVECTOR(local_lface)->contents;
  gvec = XVECTOR(global_lface)->contents;
  for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
    if (IGNORE_DEFFACE_P(gvec[i]))
      ASET(local_lface, i, Qunspecified);
    else if (!UNSPECIFIEDP(gvec[i]))
      ASET(local_lface, i, AREF(global_lface, i));

  /* If the default face was changed, update the face cache and the
     `font' frame parameter.  */
  if (EQ(face, Qdefault))
  {
    struct face_cache* c = FRAME_FACE_CACHE(f);
    struct face* newface;
    struct face* oldface = c ? FACE_FROM_ID_OR_NULL(f, DEFAULT_FACE_ID) : NULL;
    Lisp_Object attrs[LFACE_VECTOR_SIZE];

    /* This can be NULL (e.g., in batch mode).  */
    if (oldface)
    {
      /* In some cases, realize_face below can call Lisp, which could
               trigger redisplay.  But we are in the process of realizing
               the default face, and therefore are not ready to do display.  */
      specpdl_ref count = SPECPDL_INDEX();
      specbind(Qinhibit_redisplay, Qt);

      /* Ensure that the face vector is fully specified by merging
         the previously-cached vector.  */
      memcpy(attrs, oldface->lface, sizeof attrs);

      merge_face_vectors(NULL, f, lvec, attrs, 0);
      vcopy(local_lface, 0, attrs, LFACE_VECTOR_SIZE);
      newface = realize_face(c, lvec, DEFAULT_FACE_ID);

      if ((!UNSPECIFIEDP(gvec[LFACE_FAMILY_INDEX]) ||
           !UNSPECIFIEDP(gvec[LFACE_FOUNDRY_INDEX]) ||
           !UNSPECIFIEDP(gvec[LFACE_HEIGHT_INDEX]) ||
           !UNSPECIFIEDP(gvec[LFACE_WEIGHT_INDEX]) ||
           !UNSPECIFIEDP(gvec[LFACE_SLANT_INDEX]) ||
           !UNSPECIFIEDP(gvec[LFACE_SWIDTH_INDEX]) ||
           !UNSPECIFIEDP(gvec[LFACE_FONT_INDEX])) &&
          newface->font)
      {
        Lisp_Object name = newface->font->props[FONT_NAME_INDEX];
        AUTO_FRAME_ARG(arg, Qfont, name);
        Fmodify_frame_parameters(frame, arg);
      }

      if (STRINGP(gvec[LFACE_FOREGROUND_INDEX]))
      {
        AUTO_FRAME_ARG(arg, Qforeground_color, gvec[LFACE_FOREGROUND_INDEX]);
        Fmodify_frame_parameters(frame, arg);
      }

      if (STRINGP(gvec[LFACE_BACKGROUND_INDEX]))
      {
        AUTO_FRAME_ARG(arg, Qbackground_color, gvec[LFACE_BACKGROUND_INDEX]);
        Fmodify_frame_parameters(frame, arg);
      }

      unbind_to(count, Qnil);
    }
  }

  return Qnil;
}


/* The following function is implemented for compatibility with 20.2.
   The function is used in x-resolve-fonts when it is asked to
   return fonts with the same size as the font of a face.  This is
   done in fontset.el.  */

DEFUN ("face-font", Fface_font, Sface_font, 1, 3, 0,
       doc: /* Return the font name of face FACE, or nil if it is unspecified.
The font name is, by default, for ASCII characters.
If the optional argument FRAME is given, report on face FACE in that frame.
If FRAME is t, report on the defaults for face FACE (for new frames).
  The font default for a face is either nil, or a list
  of the form (bold), (italic) or (bold italic).
If FRAME is omitted or nil, use the selected frame.
If FRAME is anything but t, and the optional third argument CHARACTER
is given, return the font name used by FACE for CHARACTER on FRAME.  */)
(Lisp_Object face, Lisp_Object frame, Lisp_Object character)
{
  if (EQ(frame, Qt))
  {
    Lisp_Object result = Qnil;
    Lisp_Object lface = lface_from_face_name(NULL, face, true);

    if (!UNSPECIFIEDP(LFACE_WEIGHT(lface)) && !EQ(LFACE_WEIGHT(lface), Qnormal))
      result = Fcons(Qbold, result);

    if (!UNSPECIFIEDP(LFACE_SLANT(lface)) && !EQ(LFACE_SLANT(lface), Qnormal))
      result = Fcons(Qitalic, result);

    return result;
  }
  else
  {
    struct frame* f = decode_live_frame(frame);
    int face_id = lookup_named_face(NULL, f, face, true);
    struct face* fface = FACE_FROM_ID_OR_NULL(f, face_id);

    if (!fface)
      return Qnil;

    return build_string(FRAME_MSDOS_P(f)     ? "ms-dos"
                            : FRAME_W32_P(f) ? "w32term"
                                             : "tty");
  }
}


/* Compare face-attribute values v1 and v2 for equality.  Value is true if
   all attributes are `equal'.  Tries to be fast because this function
   is called quite often.  */

static bool face_attr_equal_p(Lisp_Object v1, Lisp_Object v2)
{
  /* Type can differ, e.g. when one attribute is unspecified, i.e. nil,
     and the other is specified.  */
  if (XTYPE(v1) != XTYPE(v2))
    return false;

  if (EQ(v1, v2))
    return true;

  switch (XTYPE(v1))
  {
  case Lisp_String:
    if (SBYTES(v1) != SBYTES(v2))
      return false;

    return memcmp(SDATA(v1), SDATA(v2), SBYTES(v1)) == 0;

  case Lisp_Int0:
  case Lisp_Int1:
  case Lisp_Symbol:
    return false;

  default:
    return !NILP(Fequal(v1, v2));
  }
}


/* Compare face vectors V1 and V2 for equality.  Value is true if
   all attributes are `equal'.  Tries to be fast because this function
   is called quite often.  */

static bool lface_equal_p(Lisp_Object* v1, Lisp_Object* v2)
{
  int i;
  bool equal_p = true;

  for (i = 1; i < LFACE_VECTOR_SIZE && equal_p; ++i)
    equal_p = face_attr_equal_p(v1[i], v2[i]);

  return equal_p;
}


DEFUN ("internal-lisp-face-equal-p", Finternal_lisp_face_equal_p,
       Sinternal_lisp_face_equal_p, 2, 3, 0,
       doc: /* True if FACE1 and FACE2 are equal.
If the optional argument FRAME is given, report on FACE1 and FACE2 in that frame.
If FRAME is t, report on the defaults for FACE1 and FACE2 (for new frames).
If FRAME is omitted or nil, use the selected frame.  */)
(Lisp_Object face1, Lisp_Object face2, Lisp_Object frame)
{
  bool equal_p;
  struct frame* f;
  Lisp_Object lface1, lface2;

  /* Don't use decode_window_system_frame here because this function
     is called before X frames exist.  At that time, if FRAME is nil,
     selected_frame will be used which is the frame dumped with
     Emacs.  That frame is not an X frame.  */
  f = EQ(frame, Qt) ? NULL : decode_live_frame(frame);

  lface1 = lface_from_face_name(f, face1, true);
  lface2 = lface_from_face_name(f, face2, true);
  equal_p = lface_equal_p(XVECTOR(lface1)->contents, XVECTOR(lface2)->contents);
  return equal_p ? Qt : Qnil;
}


DEFUN ("internal-lisp-face-empty-p", Finternal_lisp_face_empty_p,
       Sinternal_lisp_face_empty_p, 1, 2, 0,
       doc: /* True if FACE has no attribute specified.
If the optional argument FRAME is given, report on face FACE in that frame.
If FRAME is t, report on the defaults for face FACE (for new frames).
If FRAME is omitted or nil, use the selected frame.  */)
(Lisp_Object face, Lisp_Object frame)
{
  struct frame* f = EQ(frame, Qt) ? NULL : decode_live_frame(frame);
  Lisp_Object lface = lface_from_face_name(f, face, true);
  int i;

  for (i = 1; i < LFACE_VECTOR_SIZE; ++i)
    if (!UNSPECIFIEDP(AREF(lface, i)))
      break;

  return i == LFACE_VECTOR_SIZE ? Qt : Qnil;
}

DEFUN ("frame--face-hash-table", Fframe_face_hash_table, Sframe_face_hash_table,
       0, 1, 0,
       doc: /* Return a hash table of frame-local faces defined on FRAME.
For internal use only.  */)
(Lisp_Object frame) { return decode_live_frame(frame)->face_hash_table; }


/* Return a hash code for Lisp string STRING with case ignored.  Used
   below in computing a hash value for a Lisp face.  */

static uintptr_t hash_string_case_insensitive(Lisp_Object string)
{
  const unsigned char* s;
  uintptr_t hash = 0;
  eassert(STRINGP(string));
  for (s = SDATA(string); *s; ++s)
    hash = (hash << 1) ^ c_tolower(*s);
  return hash;
}


/* Return a hash code for face attribute vector V.  */

static uintptr_t lface_hash(Lisp_Object* v)
{
  return (hash_string_case_insensitive(v[LFACE_FAMILY_INDEX]) ^
          hash_string_case_insensitive(v[LFACE_FOUNDRY_INDEX]) ^
          hash_string_case_insensitive(v[LFACE_FOREGROUND_INDEX]) ^
          hash_string_case_insensitive(v[LFACE_BACKGROUND_INDEX]) ^
          XHASH(v[LFACE_WEIGHT_INDEX]) ^ XHASH(v[LFACE_SLANT_INDEX]) ^
          XHASH(v[LFACE_SWIDTH_INDEX]) ^ XHASH(v[LFACE_HEIGHT_INDEX]));
}

/***********************************************************************
          Realized Faces
 ***********************************************************************/

/* Allocate and return a new realized face for Lisp face attribute
   vector ATTR.  */

static struct face* make_realized_face(Lisp_Object* attr)
{
  enum
  {
    off = offsetof(struct face, id)
  };
  struct face* face = xmalloc(sizeof *face);

  memcpy(face->lface, attr, sizeof face->lface);
  memset(&face->id, 0, sizeof *face - off);
  face->ascii_face = face;

  return face;
}


/* Free realized face FACE, including its X resources.  FACE may
   be null.  */

static void free_realized_face(struct frame* f, struct face* face)
{
  if (face)
  {
    xfree(face);
  }
}

/* Returns the `distance' between the colors X and Y.  */

static int color_distance(Emacs_Color* x, Emacs_Color* y)
{
  /* This formula is from a paper titled `Colour metric' by Thiadmer Riemersma.
     Quoting from that paper:

   This formula has results that are very close to L*u*v* (with the
   modified lightness curve) and, more importantly, it is a more even
   algorithm: it does not have a range of colors where it suddenly
   gives far from optimal results.

     See <https://www.compuphase.com/cmetric.htm> for more info.  */

  long long r = x->red - y->red;
  long long g = x->green - y->green;
  long long b = x->blue - y->blue;
  long long r_mean = (x->red + y->red) >> 1;

  return (((((2 * 65536 + r_mean) * r * r) >> 16) + 4 * g * g +
           (((2 * 65536 + 65535 - r_mean) * b * b) >> 16)) >>
          16);
}


DEFUN ("color-distance", Fcolor_distance, Scolor_distance, 2, 4, 0,
       doc: /* Return an integer distance between COLOR1 and COLOR2 on FRAME.
COLOR1 and COLOR2 may be either strings containing the color name,
or lists of the form (RED GREEN BLUE), each in the range 0 to 65535 inclusive.
If FRAME is unspecified or nil, the current frame is used.
If METRIC is specified, it should be a function that accepts
two lists of the form (RED GREEN BLUE) aforementioned.
Despite the name, this is not a true distance metric as it does not satisfy
the triangle inequality.  */)
(Lisp_Object color1, Lisp_Object color2, Lisp_Object frame, Lisp_Object metric)
{
  struct frame* f = decode_live_frame(frame);
  Emacs_Color cdef1, cdef2;

  if (!(CONSP(color1) && parse_rgb_list(color1, &cdef1)) &&
      !(STRINGP(color1) &&
        FRAME_TERMINAL(f)->defined_color_hook(f, SSDATA(color1), &cdef1, false,
                                              true)))
    signal_error("Invalid color", color1);
  if (!(CONSP(color2) && parse_rgb_list(color2, &cdef2)) &&
      !(STRINGP(color2) &&
        FRAME_TERMINAL(f)->defined_color_hook(f, SSDATA(color2), &cdef2, false,
                                              true)))
    signal_error("Invalid color", color2);

  if (NILP(metric))
    return make_fixnum(color_distance(&cdef1, &cdef2));
  else
    return calln(metric, list3i(cdef1.red, cdef1.green, cdef1.blue),
                 list3i(cdef2.red, cdef2.green, cdef2.blue));
}


/***********************************************************************
            Face Cache
 ***********************************************************************/

/* Return a new face cache for frame F.  */

static struct face_cache* make_face_cache(struct frame* f)
{
  struct face_cache* c = xmalloc(sizeof *c);

  c->buckets = xzalloc(FACE_CACHE_BUCKETS_SIZE * sizeof *c->buckets);
  c->size = 50;
  c->used = 0;
  c->faces_by_id = xmalloc(c->size * sizeof *c->faces_by_id);
  c->f = f;
  c->menu_face_changed_p = menu_face_changed_default;
  return c;
}

/* Free all realized faces in face cache C, including basic faces.
   C may be null.  If faces are freed, make sure the frame's current
   matrix is marked invalid, so that a display caused by an expose
   event doesn't try to use faces we destroyed.  */

static void free_realized_faces(struct face_cache* c)
{
  if (c && c->used)
  {
    int i, size;
    struct frame* f = c->f;

    /* We must block input here because we can't process X events
 safely while only some faces are freed, or when the frame's
 current matrix still references freed faces.  */
    block_input();

    for (i = 0; i < c->used; ++i)
    {
      free_realized_face(f, c->faces_by_id[i]);
      c->faces_by_id[i] = NULL;
    }

    /* Forget the escape-glyph and glyphless-char faces.  */
    forget_escape_and_glyphless_faces();
    c->used = 0;
    size = FACE_CACHE_BUCKETS_SIZE * sizeof *c->buckets;
    memset(c->buckets, 0, size);

    /* Must do a thorough redisplay the next time.  Mark current
 matrices as invalid because they will reference faces freed
 above.  This function is also called when a frame is
 destroyed.  In this case, the root window of F is nil.  */
    if (WINDOWP(f->root_window))
    {
      clear_current_matrices(f);
      fset_redisplay(f);
    }

    unblock_input();
  }
}


/* Free all realized faces on FRAME or on all frames if FRAME is nil.
   This is done after attributes of a named face have been changed,
   because we can't tell which realized faces depend on that face.  */

void free_all_realized_faces(Lisp_Object frame)
{
  if (NILP(frame))
  {
    Lisp_Object rest;
    FOR_EACH_FRAME(rest, frame)
    free_realized_faces(FRAME_FACE_CACHE(XFRAME(frame)));
    windows_or_buffers_changed = 58;
  }
  else
    free_realized_faces(FRAME_FACE_CACHE(XFRAME(frame)));
}


/* Free face cache C and faces in it, including their X resources.  */

static void free_face_cache(struct face_cache* c)
{
  if (c)
  {
    free_realized_faces(c);
    xfree(c->buckets);
    xfree(c->faces_by_id);
    xfree(c);
  }
}


/* Cache realized face FACE in face cache C.  HASH is the hash value
   of FACE.  If FACE is for ASCII characters (i.e. FACE->ascii_face ==
   FACE), insert the new face to the beginning of the collision list
   of the face hash table of C.  Otherwise, add the new face to the
   end of the collision list.  This way, lookup_face can quickly find
   that a requested face is not cached.  */

static void cache_face(struct face_cache* c, struct face* face, uintptr_t hash)
{
  int i = hash % FACE_CACHE_BUCKETS_SIZE;

  face->hash = hash;

  if (face->ascii_face != face)
  {
    struct face* last = c->buckets[i];
    if (last)
    {
      while (last->next)
        last = last->next;
      last->next = face;
      face->prev = last;
      face->next = NULL;
    }
    else
    {
      c->buckets[i] = face;
      face->prev = face->next = NULL;
    }
  }
  else
  {
    face->prev = NULL;
    face->next = c->buckets[i];
    if (face->next)
      face->next->prev = face;
    c->buckets[i] = face;
  }

  /* Find a free slot in C->faces_by_id and use the index of the free
     slot as FACE->id.  */
  for (i = 0; i < c->used; ++i)
    if (c->faces_by_id[i] == NULL)
      break;
  face->id = i;

  /* Maybe enlarge C->faces_by_id.  */
  if (i == c->used)
  {
    if (c->used == c->size)
      c->faces_by_id = xpalloc(c->faces_by_id, &c->size, 1, MAX_FACE_ID,
                               sizeof *c->faces_by_id);
    c->used++;
  }

  c->faces_by_id[i] = face;
}


/* Remove face FACE from cache C.  */

static void uncache_face(struct face_cache* c, struct face* face)
{
  int i = face->hash % FACE_CACHE_BUCKETS_SIZE;

  if (face->prev)
    face->prev->next = face->next;
  else
    c->buckets[i] = face->next;

  if (face->next)
    face->next->prev = face->prev;

  c->faces_by_id[face->id] = NULL;
  if (face->id == c->used)
    --c->used;
}


/* Look up a realized face with face attributes ATTR in the face cache
   of frame F.  The face will be used to display ASCII characters.
   Value is the ID of the face found.  If no suitable face is found,
   realize a new one.  */

static int lookup_face(struct frame* f, Lisp_Object* attr)
{
  struct face_cache* cache = FRAME_FACE_CACHE(f);
  struct face* face;

  eassert(cache != NULL);
  check_lface_attrs(attr);

  /* Look up ATTR in the face cache.  */
  uintptr_t hash = lface_hash(attr);
  int i = hash % FACE_CACHE_BUCKETS_SIZE;

  for (face = cache->buckets[i]; face; face = face->next)
  {
    if (face->ascii_face != face)
    {
      /* There's no more ASCII face.  */
      face = NULL;
      break;
    }
    if (face->hash == hash && lface_equal_p(face->lface, attr))
      break;
  }

  /* If not found, realize a new face.  */
  if (face == NULL)
    face = realize_face(cache, attr, -1);

  return face->id;
}

/* Return the face id of the realized face for named face SYMBOL on
   frame F suitable for displaying ASCII characters.  Value is -1 if
   the face couldn't be determined, which might happen if the default
   face isn't realized and cannot be realized.  If window W is given,
   consider face remappings specified for W or for W's buffer.  If W
   is NULL, consider only frame-level face configuration.  */
int lookup_named_face(struct window* w, struct frame* f, Lisp_Object symbol,
                      bool signal_p)
{
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  Lisp_Object symbol_attrs[LFACE_VECTOR_SIZE];
  struct face* default_face = FACE_FROM_ID_OR_NULL(f, DEFAULT_FACE_ID);

  if (default_face == NULL)
  {
    if (!realize_basic_faces(f))
      return -1;
    default_face = FACE_FROM_ID(f, DEFAULT_FACE_ID);
  }

  if (!get_lface_attributes(w, f, symbol, symbol_attrs, signal_p, 0))
    return -1;

  memcpy(attrs, default_face->lface, sizeof attrs);

  /* Make explicit any attributes whose value is 'reset'.  */
  int i;
  for (i = 1; i < LFACE_VECTOR_SIZE; i++)
    if (EQ(symbol_attrs[i], Qreset))
      symbol_attrs[i] = attrs[i];

  merge_face_vectors(w, f, symbol_attrs, attrs, 0);

  return lookup_face(f, attrs);
}


/* Return the display face-id of the basic face whose canonical face-id
   is FACE_ID.  The return value will usually simply be FACE_ID, unless that
   basic face has been remapped via Vface_remapping_alist.  This function is
   conservative: if something goes wrong, it will simply return FACE_ID
   rather than signal an error.  Window W, if non-NULL, is used to filter
   face specifications for remapping.  */
int lookup_basic_face(struct window* w, struct frame* f, int face_id)
{
  Lisp_Object name, mapping;
  int remapped_face_id;

  if (NILP(Vface_remapping_alist))
    return face_id; /* Nothing to do.  */

  switch (face_id)
  {
  case DEFAULT_FACE_ID:
    name = Qdefault;
    break;
  case MODE_LINE_ACTIVE_FACE_ID:
    name = Qmode_line_active;
    break;
  case MODE_LINE_INACTIVE_FACE_ID:
    name = Qmode_line_inactive;
    break;
  case HEADER_LINE_ACTIVE_FACE_ID:
    name = Qheader_line_active;
    break;
  case HEADER_LINE_INACTIVE_FACE_ID:
    name = Qheader_line_inactive;
    break;
  case TAB_LINE_FACE_ID:
    name = Qtab_line;
    break;
  case TAB_BAR_FACE_ID:
    name = Qtab_bar;
    break;
  case TOOL_BAR_FACE_ID:
    name = Qtool_bar;
    break;
  case FRINGE_FACE_ID:
    name = Qfringe;
    break;
  case SCROLL_BAR_FACE_ID:
    name = Qscroll_bar;
    break;
  case BORDER_FACE_ID:
    name = Qborder;
    break;
  case CURSOR_FACE_ID:
    name = Qcursor;
    break;
  case MOUSE_FACE_ID:
    name = Qmouse;
    break;
  case MENU_FACE_ID:
    name = Qmenu;
    break;
  case WINDOW_DIVIDER_FACE_ID:
    name = Qwindow_divider;
    break;
  case VERTICAL_BORDER_FACE_ID:
    name = Qvertical_border;
    break;
  case WINDOW_DIVIDER_FIRST_PIXEL_FACE_ID:
    name = Qwindow_divider_first_pixel;
    break;
  case WINDOW_DIVIDER_LAST_PIXEL_FACE_ID:
    name = Qwindow_divider_last_pixel;
    break;
  case INTERNAL_BORDER_FACE_ID:
    name = Qinternal_border;
    break;
  case CHILD_FRAME_BORDER_FACE_ID:
    name = Qchild_frame_border;
    break;

  default:
    emacs_abort(); /* the caller is supposed to pass us a basic face id */
  }

  /* Do a quick scan through Vface_remapping_alist, and return immediately
     if there is no remapping for face NAME.  This is just an optimization
     for the very common no-remapping case.  */
  mapping = assq_no_quit(name, Vface_remapping_alist);
  if (NILP(mapping))
  {
    Lisp_Object face_attrs[LFACE_VECTOR_SIZE];

    /* If the face inherits from another, we need to realize it,
       because the parent face could be remapped.  */
    if (!get_lface_attributes(w, f, name, face_attrs, false, 0) ||
        NILP(face_attrs[LFACE_INHERIT_INDEX]) ||
        UNSPECIFIEDP(face_attrs[LFACE_INHERIT_INDEX]))
      return face_id; /* Give up.  */
  }

  /* If there is a remapping entry, or the face inherits from another,
     lookup the face using NAME, which will handle the remapping too.  */
  remapped_face_id = lookup_named_face(w, f, name, false);
  if (remapped_face_id < 0)
    return face_id; /* Give up. */

  return remapped_face_id;
}


/* Return a face for charset ASCII that is like the face with id
   FACE_ID on frame F, but has a font that is STEPS steps smaller.
   STEPS < 0 means larger.  Value is the id of the face.  */

int smaller_face(struct frame* f, int face_id, int steps)
{
  return face_id;
}


/* Return a face for charset ASCII that is like the face with id
   FACE_ID on frame F, but has height HEIGHT.  */

int face_with_height(struct frame* f, int face_id, int height)
{
  return face_id;
}


/* Return the face id of the realized face for named face SYMBOL on
   frame F suitable for displaying ASCII characters, and use
   attributes of the face FACE_ID for attributes that aren't
   completely specified by SYMBOL.  This is like lookup_named_face,
   except that the default attributes come from FACE_ID, not from the
   default face.  FACE_ID is assumed to be already realized.
   Window W, if non-NULL, filters face specifications.  */
int lookup_derived_face(struct window* w, struct frame* f, Lisp_Object symbol,
                        int face_id, bool signal_p)
{
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  Lisp_Object symbol_attrs[LFACE_VECTOR_SIZE];
  struct face* default_face;

  if (!get_lface_attributes(w, f, symbol, symbol_attrs, signal_p, 0))
    return -1;

  default_face = FACE_FROM_ID(f, face_id);
  memcpy(attrs, default_face->lface, sizeof attrs);

  /* Make explicit any attributes whose value is 'reset'.  */
  int i;
  for (i = 1; i < LFACE_VECTOR_SIZE; i++)
    if (EQ(symbol_attrs[i], Qreset))
      symbol_attrs[i] = attrs[i];

  merge_face_vectors(w, f, symbol_attrs, attrs, 0);
  return lookup_face(f, attrs);
}

DEFUN("face-attributes-as-vector", Fface_attributes_as_vector,
      Sface_attributes_as_vector, 1, 1, 0,
      doc:/* Return a vector of face attributes corresponding to PLIST.  */)
(Lisp_Object plist)
{
  Lisp_Object lface = make_vector(LFACE_VECTOR_SIZE, Qunspecified);
  merge_face_ref(NULL, XFRAME(selected_frame), plist, XVECTOR(lface)->contents,
                 true, NULL, 0);
  return lface;
}


/***********************************************************************
      Face capability testing
 ***********************************************************************/


/* If the distance (as returned by color_distance) between two colors is
   less than this, then they are considered the same, for determining
   whether a color is supported or not.  */

#define TTY_SAME_COLOR_THRESHOLD 10000

/* Return true if all the face attributes in ATTRS are supported
   on the tty frame F.

   The definition of `supported' is somewhat heuristic, but basically means
   that a face containing all the attributes in ATTRS, when merged
   with the default face for display, can be represented in a way that's

    (1) different in appearance from the default face, and
    (2) `close in spirit' to what the attributes specify, if not exact.

   Point (2) implies that a `:weight black' attribute will be satisfied
   by any terminal that can display bold, and a `:foreground "yellow"' as
   long as the terminal can display a yellowish color, but `:slant italic'
   will _not_ be satisfied by the tty display code's automatic
   substitution of a `dim' face for italic.  */

static bool tty_supports_face_attributes_p(struct frame* f,
                                           Lisp_Object attrs[LFACE_VECTOR_SIZE],
                                           struct face* def_face)
{
  int weight, slant;
  Lisp_Object val, fg, bg;
  Emacs_Color fg_tty_color, fg_std_color;
  Emacs_Color bg_tty_color, bg_std_color;
  unsigned test_caps = 0;
  Lisp_Object* def_attrs = def_face->lface;

  /* First check some easy-to-check stuff; ttys support none of the
     following attributes, so we can just return false if any are requested
     (even if `nominal' values are specified, we should still return false,
     as that will be the same value that the default face uses).  We
     consider :slant unsupportable on ttys, even though the face code
     actually `fakes' them using a dim attribute if possible.  This is
     because the faked result is too different from what the face
     specifies.  */
  if (!UNSPECIFIEDP(attrs[LFACE_FAMILY_INDEX]) ||
      !UNSPECIFIEDP(attrs[LFACE_FOUNDRY_INDEX]) ||
      !UNSPECIFIEDP(attrs[LFACE_STIPPLE_INDEX]) ||
      !UNSPECIFIEDP(attrs[LFACE_HEIGHT_INDEX]) ||
      !UNSPECIFIEDP(attrs[LFACE_SWIDTH_INDEX]) ||
      !UNSPECIFIEDP(attrs[LFACE_OVERLINE_INDEX]) ||
      !UNSPECIFIEDP(attrs[LFACE_BOX_INDEX]))
    return false;

  /* Test for terminal `capabilities' (non-color character attributes).  */

  /* font weight (bold/dim) */
  val = attrs[LFACE_WEIGHT_INDEX];
  if (!UNSPECIFIEDP(val) &&
      (weight = FONT_WEIGHT_NAME_NUMERIC(val), weight >= 0))
  {
    int def_weight = FONT_WEIGHT_NAME_NUMERIC(def_attrs[LFACE_WEIGHT_INDEX]);

    if (weight > 100)
    {
      if (def_weight > 100)
        return false; /* same as default */
      test_caps = TTY_CAP_BOLD;
    }
    else if (weight < 100)
    {
      if (def_weight < 100)
        return false; /* same as default */
      test_caps = TTY_CAP_DIM;
    }
    else if (def_weight == 100)
      return false; /* same as default */
  }

  /* font slant */
  val = attrs[LFACE_SLANT_INDEX];
  if (!UNSPECIFIEDP(val) && (slant = FONT_SLANT_NAME_NUMERIC(val), slant >= 0))
  {
    int def_slant = FONT_SLANT_NAME_NUMERIC(def_attrs[LFACE_SLANT_INDEX]);
    if (slant == 100 || slant == def_slant)
      return false; /* same as default */
    else
      test_caps |= TTY_CAP_ITALIC;
  }

  /* underlining */
  val = attrs[LFACE_UNDERLINE_INDEX];
  if (!UNSPECIFIEDP(val))
  {
    if (STRINGP(val))
      test_caps |= TTY_CAP_UNDERLINE_STYLED;
    else if (EQ(CAR_SAFE(val), QCstyle))
    {
      if (!(EQ(CAR_SAFE(CDR_SAFE(val)), Qline) ||
            EQ(CAR_SAFE(CDR_SAFE(val)), Qdouble_line) ||
            EQ(CAR_SAFE(CDR_SAFE(val)), Qwave) ||
            EQ(CAR_SAFE(CDR_SAFE(val)), Qdots) ||
            EQ(CAR_SAFE(CDR_SAFE(val)), Qdashes)))
        return false; /* Face uses an unsupported underline style.  */

      test_caps |= TTY_CAP_UNDERLINE_STYLED;
    }
    else if (face_attr_equal_p(val, def_attrs[LFACE_UNDERLINE_INDEX]))
      return false; /* same as default */
    else
      test_caps |= TTY_CAP_UNDERLINE;
  }

  /* inverse video */
  val = attrs[LFACE_INVERSE_INDEX];
  if (!UNSPECIFIEDP(val))
  {
    if (face_attr_equal_p(val, def_attrs[LFACE_INVERSE_INDEX]))
      return false; /* same as default */
    else
      test_caps |= TTY_CAP_INVERSE;
  }

  /* strike through */
  val = attrs[LFACE_STRIKE_THROUGH_INDEX];
  if (!UNSPECIFIEDP(val))
  {
    if (face_attr_equal_p(val, def_attrs[LFACE_STRIKE_THROUGH_INDEX]))
      return false; /* same as default */
    else
      test_caps |= TTY_CAP_STRIKE_THROUGH;
  }

  /* Color testing.  */

  /* Check if foreground color is close enough.  */
  fg = attrs[LFACE_FOREGROUND_INDEX];
  if (STRINGP(fg))
  {
    Lisp_Object def_fg = def_attrs[LFACE_FOREGROUND_INDEX];

    if (face_attr_equal_p(fg, def_fg))
      return false; /* same as default */
    else if (!tty_lookup_color(f, fg, &fg_tty_color, &fg_std_color))
      return false; /* not a valid color */
    else if (color_distance(&fg_tty_color, &fg_std_color) >
             TTY_SAME_COLOR_THRESHOLD)
      return false; /* displayed color is too different */
    else
    /* Make sure the color is really different from the default.  */
    {
      Emacs_Color def_fg_color;
      if (tty_lookup_color(f, def_fg, &def_fg_color, 0) &&
          (color_distance(&fg_tty_color, &def_fg_color) <=
           TTY_SAME_COLOR_THRESHOLD))
        return false;
    }
  }

  /* Check if background color is close enough.  */
  bg = attrs[LFACE_BACKGROUND_INDEX];
  if (STRINGP(bg))
  {
    Lisp_Object def_bg = def_attrs[LFACE_BACKGROUND_INDEX];

    if (face_attr_equal_p(bg, def_bg))
      return false; /* same as default */
    else if (!tty_lookup_color(f, bg, &bg_tty_color, &bg_std_color))
      return false; /* not a valid color */
    else if (color_distance(&bg_tty_color, &bg_std_color) >
             TTY_SAME_COLOR_THRESHOLD)
      return false; /* displayed color is too different */
    else
    /* Make sure the color is really different from the default.  */
    {
      Emacs_Color def_bg_color;
      if (tty_lookup_color(f, def_bg, &def_bg_color, 0) &&
          (color_distance(&bg_tty_color, &def_bg_color) <=
           TTY_SAME_COLOR_THRESHOLD))
        return false;
    }
  }

  /* If both foreground and background are requested, see if the
     distance between them is OK.  We just check to see if the distance
     between the tty's foreground and background is close enough to the
     distance between the standard foreground and background.  */
  if (STRINGP(fg) && STRINGP(bg))
  {
    int delta_delta = (color_distance(&fg_std_color, &bg_std_color) -
                       color_distance(&fg_tty_color, &bg_tty_color));
    if (delta_delta > TTY_SAME_COLOR_THRESHOLD ||
        delta_delta < -TTY_SAME_COLOR_THRESHOLD)
      return false;
  }


  /* See if the capabilities we selected above are supported, with the
     given colors.  */
  return tty_capable_p(FRAME_TTY(f), test_caps);
}


DEFUN ("display-supports-face-attributes-p",
       Fdisplay_supports_face_attributes_p, Sdisplay_supports_face_attributes_p,
       1, 2, 0,
       doc: /* Return non-nil if all the face attributes in ATTRIBUTES are supported.
The optional argument DISPLAY can be a display name, a frame, or
nil (meaning the selected frame's display).

For instance, to check whether the display supports underlining:

  (display-supports-face-attributes-p \\='(:underline t))

The definition of `supported' is somewhat heuristic, but basically means
that a face containing all the attributes in ATTRIBUTES, when merged
with the default face for display, can be represented in a way that's

 (1) different in appearance from the default face, and
 (2) `close in spirit' to what the attributes specify, if not exact.

Point (2) implies that a `:weight black' attribute will be satisfied by
any display that can display bold, and a `:foreground \"yellow\"' as long
as it can display a yellowish color, but `:slant italic' will _not_ be
satisfied by the tty display code's automatic substitution of a `dim'
face for italic.  */)
(Lisp_Object attributes, Lisp_Object display)
{
  bool supports = false;
  int i;
  Lisp_Object frame;
  struct frame* f;
  struct face* def_face;
  Lisp_Object attrs[LFACE_VECTOR_SIZE];

  if (noninteractive || !initialized)
    /* We may not be able to access low-level face information in batch
       mode, or before being dumped, and this function is not going to
       be very useful in those cases anyway, so just give up.  */
    return Qnil;

  if (NILP(display))
    frame = selected_frame;
  else if (FRAMEP(display))
    frame = display;
  else
  {
    /* Find any frame on DISPLAY.  */
    Lisp_Object tail;

    frame = Qnil;
    FOR_EACH_FRAME(tail, frame)
    if (!NILP(
            Fequal(Fcdr(Fassq(Qdisplay, XFRAME(frame)->param_alist)), display)))
      break;
  }

  CHECK_LIVE_FRAME(frame);
  f = XFRAME(frame);

  for (i = 0; i < LFACE_VECTOR_SIZE; i++)
    attrs[i] = Qunspecified;
  merge_face_ref(NULL, f, attributes, attrs, true, NULL, 0);

  def_face = FACE_FROM_ID_OR_NULL(f, DEFAULT_FACE_ID);
  if (def_face == NULL)
  {
    if (!realize_basic_faces(f))
      error("Cannot realize default face");
    def_face = FACE_FROM_ID(f, DEFAULT_FACE_ID);
  }

  /* Dispatch to the appropriate handler.  */
  if (FRAME_TERMCAP_P(f) || FRAME_MSDOS_P(f))
    supports = tty_supports_face_attributes_p(f, attrs, def_face);

  return supports ? Qt : Qnil;
}


/***********************************************************************
          Font selection
 ***********************************************************************/

DEFUN ("internal-set-font-selection-order",
       Finternal_set_font_selection_order,
       Sinternal_set_font_selection_order, 1, 1, 0,
       doc: /* Set font selection order for face font selection to ORDER.
ORDER must be a list of length 4 containing the symbols `:width',
`:height', `:weight', and `:slant'.  Face attributes appearing
first in ORDER are matched first, e.g. if `:height' appears before
`:weight' in ORDER, font selection first tries to find a font with
a suitable height, and then tries to match the font weight.
Value is ORDER.  */)
(Lisp_Object order)
{
  Lisp_Object list;
  int i;
  int indices[ARRAYELTS(font_sort_order)];

  CHECK_LIST(order);
  memset(indices, 0, sizeof indices);
  i = 0;

  for (list = order; CONSP(list) && i < ARRAYELTS(indices);
       list = XCDR(list), ++i)
  {
    Lisp_Object attr = XCAR(list);
    int xlfd;

    if (EQ(attr, QCwidth))
      xlfd = XLFD_SWIDTH;
    else if (EQ(attr, QCheight))
      xlfd = XLFD_POINT_SIZE;
    else if (EQ(attr, QCweight))
      xlfd = XLFD_WEIGHT;
    else if (EQ(attr, QCslant))
      xlfd = XLFD_SLANT;
    else
      break;

    if (indices[i] != 0)
      break;
    indices[i] = xlfd;
  }

  if (!NILP(list) || i != ARRAYELTS(indices))
    signal_error("Invalid font sort order", order);
  for (i = 0; i < ARRAYELTS(font_sort_order); ++i)
    if (indices[i] == 0)
      signal_error("Invalid font sort order", order);

  if (memcmp(indices, font_sort_order, sizeof indices) != 0)
  {
    memcpy(font_sort_order, indices, sizeof font_sort_order);
    free_all_realized_faces(Qnil);
  }

  font_update_sort_order(font_sort_order);

  return Qnil;
}


DEFUN ("internal-set-alternative-font-family-alist",
       Finternal_set_alternative_font_family_alist,
       Sinternal_set_alternative_font_family_alist, 1, 1, 0,
       doc: /* Define alternative font families to try in face font selection.
ALIST is an alist of (FAMILY ALTERNATIVE1 ALTERNATIVE2 ...) entries.
Each ALTERNATIVE is tried in order if no fonts of font family FAMILY can
be found.  Value is ALIST.  */)
(Lisp_Object alist)
{
  Lisp_Object entry, tail, tail2;

  CHECK_LIST(alist);
  alist = Fcopy_sequence(alist);
  for (tail = alist; CONSP(tail); tail = XCDR(tail))
  {
    entry = XCAR(tail);
    CHECK_LIST(entry);
    entry = Fcopy_sequence(entry);
    XSETCAR(tail, entry);
    for (tail2 = entry; CONSP(tail2); tail2 = XCDR(tail2))
      XSETCAR(tail2, Fintern(XCAR(tail2), Qnil));
  }

  Vface_alternative_font_family_alist = alist;
  free_all_realized_faces(Qnil);
  return alist;
}


DEFUN ("internal-set-alternative-font-registry-alist",
       Finternal_set_alternative_font_registry_alist,
       Sinternal_set_alternative_font_registry_alist, 1, 1, 0,
       doc: /* Define alternative font registries to try in face font selection.
ALIST is an alist of (REGISTRY ALTERNATIVE1 ALTERNATIVE2 ...) entries.
Each ALTERNATIVE is tried in order if no fonts of font registry REGISTRY can
be found.  Value is ALIST.  */)
(Lisp_Object alist)
{
  Lisp_Object entry, tail, tail2;

  CHECK_LIST(alist);
  alist = Fcopy_sequence(alist);
  for (tail = alist; CONSP(tail); tail = XCDR(tail))
  {
    entry = XCAR(tail);
    CHECK_LIST(entry);
    entry = Fcopy_sequence(entry);
    XSETCAR(tail, entry);
    for (tail2 = entry; CONSP(tail2); tail2 = XCDR(tail2))
      XSETCAR(tail2, Fdowncase(XCAR(tail2)));
  }
  Vface_alternative_font_registry_alist = alist;
  free_all_realized_faces(Qnil);
  return alist;
}


/***********************************************************************
         Face Realization
 ***********************************************************************/

/* Realize basic faces on frame F.  Value is zero if frame parameters
   of F don't contain enough information needed to realize the default
   face.  */

static bool realize_basic_faces(struct frame* f)
{
  bool success_p = false;

  /* Block input here so that we won't be surprised by an X expose
     event, for instance, without having the faces set up.  */
  block_input();

  if (realize_default_face(f))
  {
    /* Basic faces must be realized disregarding face-remapping-alist,
       since otherwise face-remapping might affect the basic faces in the
       face cache, if this function happens to be invoked with current
 buffer set to a buffer with a non-nil face-remapping-alist.  */
    specpdl_ref count = SPECPDL_INDEX();
    specbind(Qface_remapping_alist, Qnil);
    realize_named_face(f, Qmode_line_active, MODE_LINE_ACTIVE_FACE_ID);
    realize_named_face(f, Qmode_line_inactive, MODE_LINE_INACTIVE_FACE_ID);
    realize_named_face(f, Qtool_bar, TOOL_BAR_FACE_ID);
    realize_named_face(f, Qfringe, FRINGE_FACE_ID);
    realize_named_face(f, Qheader_line_active, HEADER_LINE_ACTIVE_FACE_ID);
    realize_named_face(f, Qheader_line_inactive, HEADER_LINE_INACTIVE_FACE_ID);
    realize_named_face(f, Qscroll_bar, SCROLL_BAR_FACE_ID);
    realize_named_face(f, Qborder, BORDER_FACE_ID);
    realize_named_face(f, Qcursor, CURSOR_FACE_ID);
    realize_named_face(f, Qmouse, MOUSE_FACE_ID);
    realize_named_face(f, Qmenu, MENU_FACE_ID);
    realize_named_face(f, Qvertical_border, VERTICAL_BORDER_FACE_ID);
    realize_named_face(f, Qwindow_divider, WINDOW_DIVIDER_FACE_ID);
    realize_named_face(f, Qwindow_divider_first_pixel,
                       WINDOW_DIVIDER_FIRST_PIXEL_FACE_ID);
    realize_named_face(f, Qwindow_divider_last_pixel,
                       WINDOW_DIVIDER_LAST_PIXEL_FACE_ID);
    realize_named_face(f, Qinternal_border, INTERNAL_BORDER_FACE_ID);
    realize_named_face(f, Qchild_frame_border, CHILD_FRAME_BORDER_FACE_ID);
    realize_named_face(f, Qtab_bar, TAB_BAR_FACE_ID);
    realize_named_face(f, Qtab_line, TAB_LINE_FACE_ID);
    unbind_to(count, Qnil);

    /* Reflect changes in the `menu' face in menu bars.  */
    if (FRAME_FACE_CACHE(f)->menu_face_changed_p)
    {
      FRAME_FACE_CACHE(f)->menu_face_changed_p = false;
    }

    success_p = true;
  }

  unblock_input();
  return success_p;
}


/* Realize the default face on frame F.  If the face is not fully
   specified, make it fully-specified.  Attributes of the default face
   that are not explicitly specified are taken from frame parameters.  */

static bool realize_default_face(struct frame* f)
{
  struct face_cache* c = FRAME_FACE_CACHE(f);
  Lisp_Object lface;
  Lisp_Object attrs[LFACE_VECTOR_SIZE];

  /* If the `default' face is not yet known, create it.  */
  lface = lface_from_face_name(f, Qdefault, false);
  if (NILP(lface))
  {
    Lisp_Object frame;
    XSETFRAME(frame, f);
    lface = Finternal_make_lisp_face(Qdefault, frame);
  }

  if (!FRAME_WINDOW_P(f))
  {
    ASET(lface, LFACE_FAMILY_INDEX, build_string("default"));
    ASET(lface, LFACE_FOUNDRY_INDEX, LFACE_FAMILY(lface));
    ASET(lface, LFACE_SWIDTH_INDEX, Qnormal);
    ASET(lface, LFACE_HEIGHT_INDEX, make_fixnum(1));
    if (UNSPECIFIEDP(LFACE_WEIGHT(lface)))
      ASET(lface, LFACE_WEIGHT_INDEX, Qnormal);
    if (UNSPECIFIEDP(LFACE_SLANT(lface)))
      ASET(lface, LFACE_SLANT_INDEX, Qnormal);
    if (UNSPECIFIEDP(LFACE_FONTSET(lface)))
      ASET(lface, LFACE_FONTSET_INDEX, Qnil);
  }

  if (UNSPECIFIEDP(LFACE_EXTEND(lface)))
    ASET(lface, LFACE_EXTEND_INDEX, Qnil);

  if (UNSPECIFIEDP(LFACE_UNDERLINE(lface)))
    ASET(lface, LFACE_UNDERLINE_INDEX, Qnil);

  if (UNSPECIFIEDP(LFACE_OVERLINE(lface)))
    ASET(lface, LFACE_OVERLINE_INDEX, Qnil);

  if (UNSPECIFIEDP(LFACE_STRIKE_THROUGH(lface)))
    ASET(lface, LFACE_STRIKE_THROUGH_INDEX, Qnil);

  if (UNSPECIFIEDP(LFACE_BOX(lface)))
    ASET(lface, LFACE_BOX_INDEX, Qnil);

  if (UNSPECIFIEDP(LFACE_INVERSE(lface)))
    ASET(lface, LFACE_INVERSE_INDEX, Qnil);

  if (UNSPECIFIEDP(LFACE_FOREGROUND(lface)))
  {
    /* This function is called so early that colors are not yet
 set in the frame parameter list.  */
    Lisp_Object color = Fassq(Qforeground_color, f->param_alist);

    if (CONSP(color) && STRINGP(XCDR(color)))
      ASET(lface, LFACE_FOREGROUND_INDEX, XCDR(color));
    else if (FRAME_WINDOW_P(f))
      return false;
    else if (FRAME_INITIAL_P(f) || FRAME_TERMCAP_P(f) || FRAME_MSDOS_P(f))
      ASET(lface, LFACE_FOREGROUND_INDEX, build_string(unspecified_fg));
    else
      emacs_abort();
  }

  if (UNSPECIFIEDP(LFACE_BACKGROUND(lface)))
  {
    /* This function is called so early that colors are not yet
 set in the frame parameter list.  */
    Lisp_Object color = Fassq(Qbackground_color, f->param_alist);
    if (CONSP(color) && STRINGP(XCDR(color)))
      ASET(lface, LFACE_BACKGROUND_INDEX, XCDR(color));
    else if (FRAME_WINDOW_P(f))
      return false;
    else if (FRAME_INITIAL_P(f) || FRAME_TERMCAP_P(f) || FRAME_MSDOS_P(f))
      ASET(lface, LFACE_BACKGROUND_INDEX, build_string(unspecified_bg));
    else
      emacs_abort();
  }

  if (UNSPECIFIEDP(LFACE_STIPPLE(lface)))
    ASET(lface, LFACE_STIPPLE_INDEX, Qnil);

  /* Realize the face; it must be fully-specified now.  */
  eassert(lface_fully_specified_p(XVECTOR(lface)->contents));
  check_lface(lface);
  memcpy(attrs, xvector_contents(lface), sizeof attrs);
  /* In some cases, realize_face below can call Lisp, which could
     trigger redisplay.  But we are in the process of realizing
     the default face, and therefore are not ready to do display.  */
  specpdl_ref count = SPECPDL_INDEX();
  specbind(Qinhibit_redisplay, Qt);
  struct face* face = realize_face(c, attrs, DEFAULT_FACE_ID);
  unbind_to(count, Qnil);
  (void)face;
  return true;
}


/* Realize basic faces other than the default face in face cache C.
   SYMBOL is the face name, ID is the face id the realized face must
   have.  The default face must have been realized already.  */

static void realize_named_face(struct frame* f, Lisp_Object symbol, int id)
{
  struct face_cache* c = FRAME_FACE_CACHE(f);
  Lisp_Object lface = lface_from_face_name(f, symbol, false);
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  Lisp_Object symbol_attrs[LFACE_VECTOR_SIZE];

  /* The default face must exist and be fully specified.  */
  get_lface_attributes_no_remap(f, Qdefault, attrs, true);
  check_lface_attrs(attrs);
  eassert(lface_fully_specified_p(attrs));

  /* If SYMBOL isn't know as a face, create it.  */
  if (NILP(lface))
  {
    Lisp_Object frame;
    XSETFRAME(frame, f);
    lface = Finternal_make_lisp_face(symbol, frame);
  }


  get_lface_attributes_no_remap(f, symbol, symbol_attrs, true);

  /* Handle the 'reset' pseudo-value of any attribute by replacing it
     with the corresponding value of the default face.  */
  int i;
  for (i = 1; i < LFACE_VECTOR_SIZE; i++)
    if (EQ(symbol_attrs[i], Qreset))
      symbol_attrs[i] = attrs[i];
  /* Merge SYMBOL's face with the default face.  */
  merge_face_vectors(NULL, f, symbol_attrs, attrs, 0);

  /* Realize the face.  */
  realize_face(c, attrs, id);
}


/* Realize the fully-specified face with attributes ATTRS in face
   cache CACHE for ASCII characters.  If FORMER_FACE_ID is
   non-negative, it is an ID of face to remove before caching the new
   face.  Value is a pointer to the newly created realized face.  */

static struct face* realize_face(struct face_cache* cache,
                                 Lisp_Object attrs[LFACE_VECTOR_SIZE],
                                 int former_face_id)
{
  struct face* face;

  /* LFACE must be fully specified.  */
  eassert(cache != NULL);
  check_lface_attrs(attrs);

  if (former_face_id >= 0 && cache->used > former_face_id)
  {
    /* Remove the former face.  */
    struct face* former_face = cache->faces_by_id[former_face_id];
    if (former_face)
      uncache_face(cache, former_face);
    free_realized_face(cache->f, former_face);
    SET_FRAME_GARBAGED(cache->f);
  }

  if (FRAME_WINDOW_P(cache->f))
    face = realize_gui_face(cache, attrs);
  else if (FRAME_TERMCAP_P(cache->f) || FRAME_MSDOS_P(cache->f))
    face = realize_tty_face(cache, attrs);
  else if (FRAME_INITIAL_P(cache->f))
  {
    /* Create a dummy face. */
    face = make_realized_face(attrs);
  }
  else
    emacs_abort();

  /* Insert the new face.  */
  cache_face(cache, face, lface_hash(attrs));
  return face;
}

/* Realize the fully-specified face with attributes ATTRS in face
   cache CACHE for ASCII characters.  Do it for GUI frame CACHE->f.
   If the new face doesn't share font with the default face, a
   fontname is allocated from the heap and set in `font_name' of the
   new face, but it is not yet loaded here.  Value is a pointer to the
   newly created realized face.  */

static struct face* realize_gui_face(struct face_cache* cache,
                                     Lisp_Object attrs[LFACE_VECTOR_SIZE])
{
  struct face* face = NULL;
  return face;
}


/* Map the specified color COLOR of face FACE on frame F to a tty
   color index.  IDX is one of LFACE_FOREGROUND_INDEX,
   LFACE_BACKGROUND_INDEX or LFACE_UNDERLINE_INDEX, and specifies
   which color to map.  Set *DEFAULTED to true if mapping to the
   default foreground/background colors.  */

static void map_tty_color(struct frame* f, struct face* face, Lisp_Object color,
                          enum lface_attribute_index idx, bool* defaulted)
{
  Lisp_Object frame, def;
  bool foreground_p = idx != LFACE_BACKGROUND_INDEX;
  unsigned long default_pixel =
      foreground_p ? FACE_TTY_DEFAULT_FG_COLOR : FACE_TTY_DEFAULT_BG_COLOR;
  unsigned long pixel = default_pixel;

  eassert(idx == LFACE_FOREGROUND_INDEX || idx == LFACE_BACKGROUND_INDEX ||
          idx == LFACE_UNDERLINE_INDEX);

  XSETFRAME(frame, f);

  if (STRINGP(color) && SCHARS(color) && CONSP(Vtty_defined_color_alist) &&
      (def = assoc_no_quit(color, calln(Qtty_color_alist, frame)), CONSP(def)))
  {
    /* Associations in tty-defined-color-alist are of the form
 (NAME INDEX R G B).  We need the INDEX part.  */
    pixel = XFIXNUM(XCAR(XCDR(def)));
  }

  if (pixel == default_pixel && STRINGP(color))
  {
    pixel = load_color(f, face, color, idx);
  }

  switch (idx)
  {
  case LFACE_FOREGROUND_INDEX:
    face->foreground = pixel;
    break;
  case LFACE_UNDERLINE_INDEX:
    face->underline_color = pixel;
    break;
  case LFACE_BACKGROUND_INDEX:
  default:
    face->background = pixel;
    break;
  }
}

/* Realize the fully-specified face with attributes ATTRS in face
   cache CACHE for ASCII characters.  Do it for TTY frame CACHE->f.
   Value is a pointer to the newly created realized face.  */

static struct face* realize_tty_face(struct face_cache* cache,
                                     Lisp_Object attrs[LFACE_VECTOR_SIZE])
{
  struct face* face;
  int weight, slant;
  Lisp_Object underline;
  bool face_colors_defaulted = false;
  struct frame* f = cache->f;

  /* Frame must be a termcap frame.  */
  eassert(FRAME_TERMCAP_P(cache->f) || FRAME_MSDOS_P(cache->f));

  /* Allocate a new realized face.  */
  face = make_realized_face(attrs);
#if false
  face->font_name = FRAME_MSDOS_P (cache->f) ? "ms-dos" : "tty";
#endif

  /* Map face attributes to TTY appearances.  */
  weight = FONT_WEIGHT_NAME_NUMERIC(attrs[LFACE_WEIGHT_INDEX]);
  slant = FONT_SLANT_NAME_NUMERIC(attrs[LFACE_SLANT_INDEX]);
  if (weight > 100)
    face->tty_bold_p = true;
  if (slant != 100)
    face->tty_italic_p = true;
  if (!NILP(attrs[LFACE_INVERSE_INDEX]))
    face->tty_reverse_p = true;
  if (!NILP(attrs[LFACE_STRIKE_THROUGH_INDEX]))
    face->tty_strike_through_p = true;

  /* Text underline.  */
  underline = attrs[LFACE_UNDERLINE_INDEX];
  if (NILP(underline))
  {
    face->underline = FACE_NO_UNDERLINE;
    face->underline_color = 0;
  }
  else if (EQ(underline, Qt))
  {
    face->underline = FACE_UNDERLINE_SINGLE;
    face->underline_color = 0;
  }
  else if (STRINGP(underline))
  {
    face->underline = FACE_UNDERLINE_SINGLE;
    bool underline_color_defaulted;
    map_tty_color(f, face, underline, LFACE_UNDERLINE_INDEX,
                  &underline_color_defaulted);
  }
  else if (CONSP(underline))
  {
    /* `(:color COLOR :style STYLE)'.
 STYLE being one of `line', `double-line', `wave', `dots' or `dashes'.  */
    face->underline = FACE_UNDERLINE_SINGLE;
    face->underline_color = 0;

    while (CONSP(underline))
    {
      Lisp_Object keyword, value;

      keyword = XCAR(underline);
      underline = XCDR(underline);

      if (!CONSP(underline))
        break;
      value = XCAR(underline);
      underline = XCDR(underline);

      if (EQ(keyword, QCcolor))
      {
        if (EQ(value, Qforeground_color))
          face->underline_color = 0;
        else if (STRINGP(value))
        {
          bool underline_color_defaulted;
          map_tty_color(f, face, value, LFACE_UNDERLINE_INDEX,
                        &underline_color_defaulted);
        }
      }
      else if (EQ(keyword, QCstyle))
      {
        if (EQ(value, Qline))
          face->underline = FACE_UNDERLINE_SINGLE;
        else if (EQ(value, Qdouble_line))
          face->underline = FACE_UNDERLINE_DOUBLE_LINE;
        else if (EQ(value, Qwave))
          face->underline = FACE_UNDERLINE_WAVE;
        else if (EQ(value, Qdots))
          face->underline = FACE_UNDERLINE_DOTS;
        else if (EQ(value, Qdashes))
          face->underline = FACE_UNDERLINE_DASHES;
        else
          face->underline = FACE_UNDERLINE_SINGLE;
      }
    }
  }

  /* Map color names to color indices.  */
  map_tty_color(f, face, face->lface[LFACE_FOREGROUND_INDEX],
                LFACE_FOREGROUND_INDEX, &face_colors_defaulted);
  map_tty_color(f, face, face->lface[LFACE_BACKGROUND_INDEX],
                LFACE_BACKGROUND_INDEX, &face_colors_defaulted);

  /* Swap colors if face is inverse-video.  If the colors are taken
     from the frame colors, they are already inverted, since the
     frame-creation function calls x-handle-reverse-video.  */
  if (face->tty_reverse_p && !face_colors_defaulted)
  {
    unsigned long tem = face->foreground;
    face->foreground = face->background;
    face->background = tem;
  }

  if (tty_suppress_bold_inverse_default_colors_p && face->tty_bold_p &&
      face->background == FACE_TTY_DEFAULT_FG_COLOR &&
      face->foreground == FACE_TTY_DEFAULT_BG_COLOR)
    face->tty_bold_p = false;

  return face;
}


DEFUN ("tty-suppress-bold-inverse-default-colors",
       Ftty_suppress_bold_inverse_default_colors,
       Stty_suppress_bold_inverse_default_colors, 1, 1, 0,
       doc: /* Suppress/allow boldness of faces with inverse default colors.
SUPPRESS non-nil means suppress it.
This affects bold faces on TTYs whose foreground is the default background
color of the display and whose background is the default foreground color.
For such faces, the bold face attribute is ignored if this variable
is non-nil.  */)
(Lisp_Object suppress)
{
  tty_suppress_bold_inverse_default_colors_p = !NILP(suppress);
  face_change = true;
  return suppress;
}


/***********************************************************************
         Computing Faces
 ***********************************************************************/

/* Return the ID of the face to use to display character CH with face
   property PROP on frame F in current_buffer.  */

int compute_char_face(struct frame* f, int ch, Lisp_Object prop)
{
  int face_id;

  if (NILP(BVAR(current_buffer, enable_multibyte_characters)))
    ch = 0;

  if (NILP(prop))
  {
    struct face* face = FACE_FROM_ID(f, DEFAULT_FACE_ID);
    face_id = FACE_FOR_CHAR(f, face, ch, -1, Qnil);
  }
  else
  {
    Lisp_Object attrs[LFACE_VECTOR_SIZE];
    struct face* default_face = FACE_FROM_ID(f, DEFAULT_FACE_ID);
    memcpy(attrs, default_face->lface, sizeof attrs);
    merge_face_ref(NULL, f, prop, attrs, true, NULL, 0);
    face_id = lookup_face(f, attrs);
  }

  return face_id;
}

/* Return the face ID associated with buffer position POS for
   displaying ASCII characters.  Return in *ENDPTR the position at
   which a different face is needed, as far as text properties and
   overlays are concerned.  W is a window displaying current_buffer.

   ATTR_FILTER is passed merge_face_ref.

   LIMIT is a position not to scan beyond.  That is to limit the time
   this function can take.

   If MOUSE, use the character's mouse-face, not its face, and only
   consider the highest-priority source of mouse-face at POS,
   i.e. don't merge different mouse-face values if more than one
   source specifies it.

   BASE_FACE_ID, if non-negative, specifies a base face ID to use
   instead of DEFAULT_FACE_ID.

   Set *ENDPTR to the next position where to check for face or
   mouse-face.

   The face returned is suitable for displaying ASCII characters.  */

int face_at_buffer_position(struct window* w, ptrdiff_t pos, ptrdiff_t* endptr,
                            ptrdiff_t limit, bool mouse, int base_face_id,
                            enum lface_attribute_index attr_filter)
{
  struct frame* f = XFRAME(w->frame);
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  Lisp_Object prop, position;
  ptrdiff_t i, noverlays;
  Lisp_Object* overlay_vec;
  ptrdiff_t endpos;
  Lisp_Object propname = mouse ? Qmouse_face : Qface;
  Lisp_Object limit1, end;
  struct face* default_face;

  /* W must display the current buffer.  We could write this function
     to use the frame and buffer of W, but right now it doesn't.  */
  /* eassert (XBUFFER (w->contents) == current_buffer); */

  XSETFASTINT(position, pos);

  endpos = ZV;

  /* Get the `face' or `mouse_face' text property at POS, and
     determine the next position at which the property changes.  */
  prop = Fget_text_property(position, propname, w->contents);
  XSETFASTINT(limit1, min(limit, endpos));
  end = Fnext_single_property_change(position, propname, w->contents, limit1);
  if (FIXNUMP(end))
    endpos = XFIXNUM(end);

  /* Look at properties from overlays.  */
  USE_SAFE_ALLOCA;
  {
    ptrdiff_t next_overlay;
    GET_OVERLAYS_AT(pos, overlay_vec, noverlays, &next_overlay);
    /* overlays_at can return next_overlay beyond the end of the current
       narrowing.  We don't want that to leak into the display code.  */
    if (next_overlay > ZV)
      next_overlay = ZV;
    if (next_overlay < endpos)
      endpos = next_overlay;
  }

  *endptr = endpos;

  {
    int face_id;

    if (base_face_id >= 0)
      face_id = base_face_id;
    else if (NILP(Vface_remapping_alist))
      face_id = DEFAULT_FACE_ID;
    else
      face_id = lookup_basic_face(w, f, DEFAULT_FACE_ID);

    default_face = FACE_FROM_ID_OR_NULL(f, face_id);
    /* Make sure the default face ID is usable: if someone freed the
       cached faces since we've looked up these faces, we need to look
       them up again.  */
    if (!default_face)
    {
      if (FRAME_FACE_CACHE(f)->used == 0)
        recompute_basic_faces(f);
      default_face = FACE_FROM_ID(f, lookup_basic_face(w, f, DEFAULT_FACE_ID));
    }
  }

  /* Optimize common cases where we can use the default face.  */
  if (noverlays == 0 && NILP(prop))
  {
    SAFE_FREE();
    return default_face->id;
  }

  /* Begin with attributes from the default face.  */
  memcpy(attrs, default_face->lface, sizeof(attrs));

  /* Merge in attributes specified via text properties.  */
  if (!NILP(prop))
    merge_face_ref(w, f, prop, attrs, true, NULL, attr_filter);

  /* Now merge the overlay data.  */
  noverlays = sort_overlays(overlay_vec, noverlays, w);
  /* For mouse-face, we need only the single highest-priority face
     from the overlays, if any.  */
  if (mouse)
  {
    for (prop = Qnil, i = noverlays - 1; i >= 0 && NILP(prop); --i)
    {
      ptrdiff_t oendpos;

      prop = Foverlay_get(overlay_vec[i], propname);
      if (!NILP(prop))
      {
        /* Overlays always take priority over text properties,
     so discard the mouse-face text property, if any, and
     use the overlay property instead.  */
        memcpy(attrs, default_face->lface, sizeof attrs);
        merge_face_ref(w, f, prop, attrs, true, NULL, attr_filter);
      }

      oendpos = OVERLAY_END(overlay_vec[i]);
      if (oendpos < endpos)
        endpos = oendpos;
    }
  }
  else
  {
    for (i = 0; i < noverlays; i++)
    {
      ptrdiff_t oendpos;

      prop = Foverlay_get(overlay_vec[i], propname);

      if (!NILP(prop))
        merge_face_ref(w, f, prop, attrs, true, NULL, attr_filter);

      oendpos = OVERLAY_END(overlay_vec[i]);
      if (oendpos < endpos)
        endpos = oendpos;
    }
  }

  *endptr = endpos;

  SAFE_FREE();

  /* Look up a realized face with the given face attributes,
     or realize a new one for ASCII characters.  */
  return lookup_face(f, attrs);
}

/* Return the face ID at buffer position POS for displaying ASCII
   characters associated with overlay strings for overlay OVERLAY.

   Like face_at_buffer_position except for OVERLAY.  Currently it
   simply disregards the `face' properties of all overlays.  */

int face_for_overlay_string(struct window* w, ptrdiff_t pos, ptrdiff_t* endptr,
                            ptrdiff_t limit, bool mouse, Lisp_Object overlay,
                            enum lface_attribute_index attr_filter)
{
  struct frame* f = XFRAME(w->frame);
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  Lisp_Object prop, position;
  ptrdiff_t endpos;
  Lisp_Object propname = mouse ? Qmouse_face : Qface;
  Lisp_Object limit1, end;
  struct face* default_face;

  /* W must display the current buffer.  We could write this function
     to use the frame and buffer of W, but right now it doesn't.  */
  /* eassert (XBUFFER (w->contents) == current_buffer); */

  XSETFASTINT(position, pos);

  endpos = ZV;

  /* Get the `face' or `mouse_face' text property at POS, and
     determine the next position at which the property changes.  */
  prop = Fget_text_property(position, propname, w->contents);
  XSETFASTINT(limit1, min(limit, endpos));
  end = Fnext_single_property_change(position, propname, w->contents, limit1);
  if (FIXNUMP(end))
    endpos = XFIXNUM(end);

  *endptr = endpos;

  /* Optimize common case where we can use the default face.  */
  if (NILP(prop) && NILP(Vface_remapping_alist))
    return DEFAULT_FACE_ID;

  /* Begin with attributes from the default face.  */
  default_face = FACE_FROM_ID(f, lookup_basic_face(w, f, DEFAULT_FACE_ID));
  memcpy(attrs, default_face->lface, sizeof attrs);

  /* Merge in attributes specified via text properties.  */
  if (!NILP(prop))
    merge_face_ref(w, f, prop, attrs, true, NULL, attr_filter);

  *endptr = endpos;

  /* Look up a realized face with the given face attributes,
     or realize a new one for ASCII characters.  */
  return lookup_face(f, attrs);
}


/* Compute the face at character position POS in Lisp string STRING on
   window W, for ASCII characters.

   If STRING is an overlay string, it comes from position BUFPOS in
   current_buffer, otherwise BUFPOS is zero to indicate that STRING is
   not an overlay string.  W must display the current buffer.

   BASE_FACE_ID is the id of a face to merge with.  For strings coming
   from overlays or the `display' property it is the face at BUFPOS.

   If MOUSE_P, use the character's mouse-face, not its face.

   Set *ENDPTR to the next position where to check for faces in
   STRING; -1 if the face is constant from POS to the end of the
   string.

   Value is the id of the face to use.  The face returned is suitable
   for displaying ASCII characters.  */

int face_at_string_position(struct window* w, Lisp_Object string, ptrdiff_t pos,
                            ptrdiff_t bufpos, ptrdiff_t* endptr,
                            enum face_id base_face_id, bool mouse_p,
                            enum lface_attribute_index attr_filter)
{
  Lisp_Object prop, position, end, limit;
  struct frame* f = XFRAME(WINDOW_FRAME(w));
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  struct face* base_face;
  bool multibyte_p = STRING_MULTIBYTE(string);
  Lisp_Object prop_name = mouse_p ? Qmouse_face : Qface;

  /* Get the value of the face property at the current position within
     STRING.  Value is nil if there is no face property.  */
  XSETFASTINT(position, pos);
  prop = Fget_text_property(position, prop_name, string);

  /* Get the next position at which to check for faces.  Value of end
     is nil if face is constant all the way to the end of the string.
     Otherwise it is a string position where to check faces next.
     Limit is the maximum position up to which to check for property
     changes in Fnext_single_property_change.  Strings are usually
     short, so set the limit to the end of the string.  */
  XSETFASTINT(limit, SCHARS(string));
  end = Fnext_single_property_change(position, prop_name, string, limit);
  if (FIXNUMP(end))
    *endptr = XFIXNAT(end);
  else
    *endptr = -1;

  base_face = FACE_FROM_ID_OR_NULL(f, base_face_id);
  if (!base_face)
    base_face = FACE_FROM_ID(f, lookup_basic_face(w, f, DEFAULT_FACE_ID));

  /* Optimize the default case that there is no face property.  */
  if (NILP(prop) &&
      (multibyte_p
       /* We can't realize faces for different charsets differently
          if we don't have fonts, so we can stop here if not working
          on a window-system frame.  */
       || !FRAME_WINDOW_P(f) || FACE_SUITABLE_FOR_ASCII_CHAR_P(base_face)))
    return base_face->id;

  /* Begin with attributes from the base face.  */
  memcpy(attrs, base_face->lface, sizeof attrs);

  /* Merge in attributes specified via text properties.  */
  if (!NILP(prop))
    merge_face_ref(w, f, prop, attrs, true, NULL, attr_filter);

  /* Look up a realized face with the given face attributes,
     or realize a new one for ASCII characters.  */
  return lookup_face(f, attrs);
}


/* Merge a face into a realized face.

   W is a window in the frame where faces are (to be) realized.

   FACE_NAME is named face to merge.

   If FACE_NAME is nil, FACE_ID is face_id of realized face to merge.

   If FACE_NAME is t, FACE_ID is lface_id of face to merge.

   BASE_FACE_ID is realized face to merge into.

   Return new face id.
*/

int merge_faces(struct window* w, Lisp_Object face_name, int face_id,
                int base_face_id)
{
  struct frame* f = WINDOW_XFRAME(w);
  Lisp_Object attrs[LFACE_VECTOR_SIZE];
  struct face* base_face = FACE_FROM_ID_OR_NULL(f, base_face_id);

  if (!base_face)
    return base_face_id;

  if (EQ(face_name, Qt))
  {
    if (face_id < 0 || face_id >= lface_id_to_name_size)
      return base_face_id;
    face_name = lface_id_to_name[face_id];
    /* When called during make-frame, lookup_derived_face may fail
 if the faces are uninitialized.  Don't signal an error.  */
    face_id = lookup_derived_face(w, f, face_name, base_face_id, 0);
    return (face_id >= 0 ? face_id : base_face_id);
  }

  /* Begin with attributes from the base face.  */
  memcpy(attrs, base_face->lface, sizeof attrs);

  if (!NILP(face_name))
  {
    if (!merge_named_face(w, f, face_name, attrs, NULL, 0))
      return base_face_id;
  }
  else
  {
    if (face_id < 0)
      return base_face_id;

    struct face* face = FACE_FROM_ID_OR_NULL(f, face_id);

    if (!face)
      return base_face_id;

    if (face_id != DEFAULT_FACE_ID)
    {
      struct face* deflt = FACE_FROM_ID(f, DEFAULT_FACE_ID);
      Lisp_Object lface_attrs[LFACE_VECTOR_SIZE];
      int i;

      memcpy(lface_attrs, face->lface, LFACE_VECTOR_SIZE);
      /* Make explicit any attributes whose value is 'reset'.  */
      for (i = 1; i < LFACE_VECTOR_SIZE; i++)
        if (EQ(lface_attrs[i], Qreset))
          lface_attrs[i] = deflt->lface[i];
      merge_face_vectors(w, f, lface_attrs, attrs, 0);
    }
    else
      merge_face_vectors(w, f, face->lface, attrs, 0);
  }

  /* Look up a realized face with the given face attributes,
     or realize a new one for ASCII characters.  */
  return lookup_face(f, attrs);
}

DEFUN ("x-load-color-file", Fx_load_color_file,
       Sx_load_color_file, 1, 1, 0,
       doc: /* Create an alist of color entries from an external file.

The file should define one named RGB color per line like so:
  R G B   name
where R,G,B are numbers between 0 and 255 and name is an arbitrary string.  */)
(Lisp_Object filename)
{
  FILE* fp;
  Lisp_Object cmap = Qnil;
  Lisp_Object abspath;

  CHECK_STRING(filename);
  abspath = Fexpand_file_name(filename, Qnil);

  block_input();
  fp = emacs_fopen(SSDATA(abspath), "r" FOPEN_TEXT);
  if (fp)
  {
    char buf[512];
    int red, green, blue;
    int num;

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
      if (sscanf(buf, "%d %d %d %n", &red, &green, &blue, &num) == 3)
      {
        int color = (red << 16) | (green << 8) | blue;
        char* name = buf + num;
        ptrdiff_t len = strlen(name);
        len -= 0 < len && name[len - 1] == '\n';
        cmap = Fcons(Fcons(make_string(name, len), make_fixnum(color)), cmap);
      }
    }
    emacs_fclose(fp);
  }
  unblock_input();
  return cmap;
}

/***********************************************************************
          Initialization
 ***********************************************************************/

/* All the faces defined during loadup are recorded in
   face-new-frame-defaults.  We need to set next_lface_id to the next
   face ID number, so that any new faces defined in this session will
   have face IDs different from those defined during loadup.  We also
   need to set up the lface_id_to_name[] array for the faces that were
   defined during loadup.  */
void init_xfaces(void)
{
#ifdef HAVE_PDUMPER
  int nfaces;

  if (dumped_with_pdumper_p())
  {
    nfaces = XFIXNAT(Fhash_table_count(Vface_new_frame_defaults));
    if (nfaces > 0)
    {
      /* Allocate the lface_id_to_name[] array.  */
      lface_id_to_name_size = next_lface_id = nfaces;
      lface_id_to_name = xnmalloc(next_lface_id, sizeof *lface_id_to_name);

      /* Store the faces.  */
      struct Lisp_Hash_Table* table = XHASH_TABLE(Vface_new_frame_defaults);
      for (ptrdiff_t idx = 0; idx < nfaces; ++idx)
      {
        Lisp_Object lface = HASH_KEY(table, idx);
        Lisp_Object face_id = CAR(HASH_VALUE(table, idx));
        if (FIXNATP(face_id))
        {
          int id = XFIXNAT(face_id);
          eassert(id >= 0);
          lface_id_to_name[id] = lface;
        }
      }
    }
  }
#endif

  face_attr_sym[0] = Qface;
  face_attr_sym[LFACE_FAMILY_INDEX] = QCfamily;
  face_attr_sym[LFACE_FOUNDRY_INDEX] = QCfoundry;
  face_attr_sym[LFACE_SWIDTH_INDEX] = QCwidth;
  face_attr_sym[LFACE_HEIGHT_INDEX] = QCheight;
  face_attr_sym[LFACE_WEIGHT_INDEX] = QCweight;
  face_attr_sym[LFACE_SLANT_INDEX] = QCslant;
  face_attr_sym[LFACE_UNDERLINE_INDEX] = QCunderline;
  face_attr_sym[LFACE_INVERSE_INDEX] = QCinverse_video;
  face_attr_sym[LFACE_FOREGROUND_INDEX] = QCforeground;
  face_attr_sym[LFACE_BACKGROUND_INDEX] = QCbackground;
  face_attr_sym[LFACE_STIPPLE_INDEX] = QCstipple;
  face_attr_sym[LFACE_OVERLINE_INDEX] = QCoverline;
  face_attr_sym[LFACE_STRIKE_THROUGH_INDEX] = QCstrike_through;
  face_attr_sym[LFACE_BOX_INDEX] = QCbox;
  face_attr_sym[LFACE_FONT_INDEX] = QCfont;
  face_attr_sym[LFACE_INHERIT_INDEX] = QCinherit;
  face_attr_sym[LFACE_FONTSET_INDEX] = QCfontset;
  face_attr_sym[LFACE_DISTANT_FOREGROUND_INDEX] = QCdistant_foreground;
  face_attr_sym[LFACE_EXTEND_INDEX] = QCextend;
}

void syms_of_xfaces(void)
{
  /* The symbols `face' and `mouse-face' used as text properties.  */
  DEFSYM(Qface, "face");

  /* Property for basic faces which other faces cannot inherit.  */
  DEFSYM(Qface_no_inherit, "face-no-inherit");

  /* Error symbol for wrong_type_argument in load_pixmap.  */
  DEFSYM(Qbitmap_spec_p, "bitmap-spec-p");

  /* The name of the function to call when the background of the frame
     has changed, frame_set_background_mode.  */
  DEFSYM(Qframe_set_background_mode, "frame-set-background-mode");

  /* Lisp face attribute keywords.  */
  DEFSYM(QCfamily, ":family");
  DEFSYM(QCheight, ":height");
  DEFSYM(QCweight, ":weight");
  DEFSYM(QCslant, ":slant");
  DEFSYM(QCunderline, ":underline");
  DEFSYM(QCinverse_video, ":inverse-video");
  DEFSYM(QCforeground, ":foreground");
  DEFSYM(QCbackground, ":background");
  DEFSYM(QCstipple, ":stipple");
  DEFSYM(QCwidth, ":width");
  DEFSYM(QCfont, ":font");
  DEFSYM(QCfontset, ":fontset");
  DEFSYM(QCdistant_foreground, ":distant-foreground");
  DEFSYM(QCbold, ":bold");
  DEFSYM(QCitalic, ":italic");
  DEFSYM(QCoverline, ":overline");
  DEFSYM(QCstrike_through, ":strike-through");
  DEFSYM(QCbox, ":box");
  DEFSYM(QCinherit, ":inherit");
  DEFSYM(QCextend, ":extend");

  /* Symbols used for Lisp face attribute values.  */
  DEFSYM(QCcolor, ":color");
  DEFSYM(QCline_width, ":line-width");
  DEFSYM(QCstyle, ":style");
  DEFSYM(QCposition, ":position");
  DEFSYM(Qline, "line");
  DEFSYM(Qwave, "wave");
  DEFSYM(Qdouble_line, "double-line");
  DEFSYM(Qdots, "dots");
  DEFSYM(Qdashes, "dashes");
  DEFSYM(Qreleased_button, "released-button");
  DEFSYM(Qpressed_button, "pressed-button");
  DEFSYM(Qflat_button, "flat-button");
  DEFSYM(Qnormal, "normal");
  DEFSYM(Qthin, "thin");
  DEFSYM(Qextra_light, "extra-light");
  DEFSYM(Qultra_light, "ultra-light");
  DEFSYM(Qlight, "light");
  DEFSYM(Qsemi_light, "semi-light");
  DEFSYM(Qmedium, "medium");
  DEFSYM(Qsemi_bold, "semi-bold");
  DEFSYM(Qbook, "book");
  DEFSYM(Qbold, "bold");
  DEFSYM(Qextra_bold, "extra-bold");
  DEFSYM(Qultra_bold, "ultra-bold");
  DEFSYM(Qheavy, "heavy");
  DEFSYM(Qultra_heavy, "ultra-heavy");
  DEFSYM(Qblack, "black");
  DEFSYM(Qoblique, "oblique");
  DEFSYM(Qitalic, "italic");
  DEFSYM(Qreset, "reset");

  /* The symbols `foreground-color' and `background-color' which can be
     used as part of a `face' property.  This is for compatibility with
     Emacs 20.2.  */
  DEFSYM(Qbackground_color, "background-color");
  DEFSYM(Qforeground_color, "foreground-color");

  DEFSYM(Qunspecified, "unspecified");
  DEFSYM(QCignore_defface, ":ignore-defface");

  /* Used for limiting character attributes to windows with specific
     characteristics.  */
  DEFSYM(QCwindow, ":window");
  DEFSYM(QCfiltered, ":filtered");

  /* The symbol `face-alias'.  A symbol having that property is an
     alias for another face.  Value of the property is the name of
     the aliased face.  */
  DEFSYM(Qface_alias, "face-alias");

  /* Names of basic faces.  */
  DEFSYM(Qdefault, "default");
  DEFSYM(Qtool_bar, "tool-bar");
  DEFSYM(Qtab_bar, "tab-bar");
  DEFSYM(Qfringe, "fringe");
  DEFSYM(Qtab_line, "tab-line");
  DEFSYM(Qheader_line, "header-line");
  DEFSYM(Qheader_line_inactive, "header-line-inactive");
  DEFSYM(Qheader_line_active, "header-line-active");
  DEFSYM(Qscroll_bar, "scroll-bar");
  DEFSYM(Qmenu, "menu");
  DEFSYM(Qcursor, "cursor");
  DEFSYM(Qborder, "border");
  DEFSYM(Qmouse, "mouse");
  DEFSYM(Qmode_line_inactive, "mode-line-inactive");
  DEFSYM(Qmode_line_active, "mode-line-active");
  DEFSYM(Qvertical_border, "vertical-border");
  DEFSYM(Qwindow_divider, "window-divider");
  DEFSYM(Qwindow_divider_first_pixel, "window-divider-first-pixel");
  DEFSYM(Qwindow_divider_last_pixel, "window-divider-last-pixel");
  DEFSYM(Qinternal_border, "internal-border");
  DEFSYM(Qchild_frame_border, "child-frame-border");

  /* TTY color-related functions (defined in tty-colors.el).  */
  DEFSYM(Qtty_color_desc, "tty-color-desc");
  DEFSYM(Qtty_color_standard_values, "tty-color-standard-values");
  DEFSYM(Qtty_color_by_index, "tty-color-by-index");

  /* The name of the function used to compute colors on TTYs.  */
  DEFSYM(Qtty_color_alist, "tty-color-alist");
  DEFSYM(Qtty_defined_color_alist, "tty-defined-color-alist");

  Vface_alternative_font_family_alist = Qnil;
  staticpro(&Vface_alternative_font_family_alist);
  Vface_alternative_font_registry_alist = Qnil;
  staticpro(&Vface_alternative_font_registry_alist);

  defsubr(&Sinternal_make_lisp_face);
  defsubr(&Sinternal_lisp_face_p);
  defsubr(&Sinternal_set_lisp_face_attribute);
  defsubr(&Scolor_gray_p);
  defsubr(&Scolor_supported_p);
  defsubr(&Sface_attribute_relative_p);
  defsubr(&Smerge_face_attribute);
  defsubr(&Sinternal_get_lisp_face_attribute);
  defsubr(&Sinternal_lisp_face_attribute_values);
  defsubr(&Sinternal_lisp_face_equal_p);
  defsubr(&Sinternal_lisp_face_empty_p);
  defsubr(&Sinternal_copy_lisp_face);
  defsubr(&Sinternal_merge_in_global_face);
  defsubr(&Sface_font);
  defsubr(&Sframe_face_hash_table);
  defsubr(&Sdisplay_supports_face_attributes_p);
  defsubr(&Scolor_distance);
  defsubr(&Sinternal_set_font_selection_order);
  defsubr(&Sinternal_set_alternative_font_family_alist);
  defsubr(&Sinternal_set_alternative_font_registry_alist);
  defsubr(&Sface_attributes_as_vector);
  defsubr(&Sclear_face_cache);
  defsubr(&Stty_suppress_bold_inverse_default_colors);

  DEFVAR_BOOL ("face-filters-always-match", face_filters_always_match,
    doc: /* Non-nil means that face filters are always deemed to match.
This variable is intended for use only by code that evaluates
the "specificity" of a face specification and should be let-bound
only for this purpose.  */);

  DEFVAR_LISP("face--new-frame-defaults", Vface_new_frame_defaults,
              doc
:/* Hash table of global face definitions (for internal use only.)  */);
  Vface_new_frame_defaults =
      /* 33 entries is enough to fit all basic faces */
      make_hash_table(&hashtest_eq, 33, Weak_None);

  DEFVAR_LISP ("face-default-stipple", Vface_default_stipple,
    doc: /* Default stipple pattern used on monochrome displays.
This stipple pattern is used on monochrome displays
instead of shades of gray for a face background color.
See `set-face-stipple' for possible values for this variable.  */);
  Vface_default_stipple = build_string("gray3");

  DEFVAR_LISP ("tty-defined-color-alist", Vtty_defined_color_alist,
   doc: /* An alist of defined terminal colors and their RGB values.
See the docstring of `tty-color-alist' for the details.  */);
  Vtty_defined_color_alist = Qnil;

  DEFVAR_LISP ("scalable-fonts-allowed", Vscalable_fonts_allowed,
	       doc: /* Allowed scalable fonts.
A value of nil means don't allow any scalable fonts.
A value of t means allow any scalable font.
Otherwise, value must be a list of regular expressions.  A font may be
scaled if its name matches a regular expression in the list.
Note that if value is nil, a scalable font might still be used, if no
other font of the appropriate family and registry is available.  */);
  Vscalable_fonts_allowed = Qnil;

  DEFVAR_LISP("face-ignored-fonts", Vface_ignored_fonts, doc:
                  /* List of ignored fonts.
    Each element is a regular expression that matches names of fonts to
    ignore.  */);

  Vface_ignored_fonts = Qnil;

  DEFVAR_LISP ("face-remapping-alist", Vface_remapping_alist,
	       doc: /* Alist of face remappings.
Each element is of the form:

   (FACE . REPLACEMENT),

which causes display of the face FACE to use REPLACEMENT instead.
REPLACEMENT is a face specification, i.e. one of the following:

  (1) a face name
  (2) a property list of attribute/value pairs, or
  (3) a list in which each element has one of the above forms.

List values for REPLACEMENT are merged to form the final face
specification, with earlier entries taking precedence, in the same way
as with the `face' text property.

Face-name remapping cycles are suppressed; recursive references use
the underlying face instead of the remapped face.  So a remapping of
the form:

   (FACE EXTRA-FACE... FACE)

or:

   (FACE (FACE-ATTR VAL ...) FACE)

causes EXTRA-FACE... or (FACE-ATTR VAL ...) to be _merged_ with the
existing definition of FACE.  Note that this isn't necessary for the
default face, since every face inherits from the default face.

An entry in the list can also be a filtered face expression of the
form:

  (:filtered FILTER FACE-SPECIFICATION)

This construct applies FACE-SPECIFICATION (which can have any of the
forms allowed for face specifications generally) only if FILTER
matches at the moment Emacs wants to draw text with the combined face.

The only filters currently defined are NIL (which always matches) and
(:window PARAMETER VALUE), which matches only in the context of a
window with a parameter EQ-equal to VALUE.

An entry in the face list can also be nil, which does nothing.

If `face-remapping-alist' is made buffer-local, the face remapping
takes effect only in that buffer.  For instance, the mode my-mode
could define a face `my-mode-default', and then in the mode setup
function, do:

   (set (make-local-variable \\='face-remapping-alist)
        (copy-tree \\='((default my-mode-default)))).

You probably want to use the face-remap package included in Emacs
instead of manipulating face-remapping-alist directly.  Note that many
of the functions in that package modify the list destructively, so make
sure you set it to a fresh value (for instance, use `copy-tree' as in
the example above) before modifying.

Because Emacs normally only redraws screen areas when the underlying
buffer contents change, you may need to call `redraw-display' after
changing this variable for it to take effect.  */);
  Vface_remapping_alist = Qnil;
  DEFSYM(Qface_remapping_alist, "face-remapping-alist");

  DEFVAR_LISP ("face-font-rescale-alist", Vface_font_rescale_alist,
	       doc: /* Alist of fonts vs the rescaling factors.
Each element is a cons (FONT-PATTERN . RESCALE-RATIO), where
FONT-PATTERN is a font-spec or a regular expression matching a font name, and
RESCALE-RATIO is a floating point number to specify how much larger
\(or smaller) font we should use.  For instance, if a face requests
a font of 10 point, we actually use a font of 10 * RESCALE-RATIO point.  */);
  Vface_font_rescale_alist = Qnil;

  DEFVAR_INT ("face-near-same-color-threshold", face_near_same_color_threshold,
	      doc: /* Threshold for using distant-foreground color instead of foreground.

The value should be an integer number providing the minimum distance
between two colors that will still qualify them to be used as foreground
and background.  If the value of `color-distance', invoked with a nil
METRIC argument, for the foreground and background colors of a face is
less than this threshold, the distant-foreground color, if defined,
will be used for the face instead of the foreground color.

Lisp programs that change the value of this variable should also
clear the face cache, see `clear-face-cache'.  */);
  face_near_same_color_threshold = 30000;

  DEFVAR_LISP ("face-font-lax-matched-attributes",
	       Vface_font_lax_matched_attributes,
	       doc: /* Whether to match some face attributes in lax manner when realizing faces.

If non-nil, some font-related face attributes will be matched in a lax
manner when looking for candidate fonts.
If the value is t, the default, the search for fonts will not insist
on exact match for 3 font attributes: weight, width, and slant.
Instead, it will examine the available fonts with various values of
these attributes, and select the font that is the closest possible
match.  (If an exact match is available, it will still be selected,
as that is the closest match.)  For example, looking for a semi-bold
font might select a bold or a medium-weight font if no semi-bold font
matching other attributes can be found.  This is especially important
when the `default' face specifies unusual values for one or more of
these 3 attributes, which other installed fonts don't support.

The value can also be a list of font-related face attribute symbols;
see `set-face-attribute' for the full list of attributes.  Then the
corresponding face attributes will be treated as "soft" constraints
in the manner described above, instead of the default 3 attributes.

If the value is nil, candidate fonts might be rejected if the don't
have exactly the same values of attributes as the face requests.

This variable exists for debugging of the font-selection process,
and we advise not to change it otherwise.  */);
  Vface_font_lax_matched_attributes = Qt;

  defsubr(&Scolor_values_from_color_spec);
}
