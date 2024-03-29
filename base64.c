/*
 *     This program is free software; you can redistribute it
 *     and/or modify it under the terms of the GNU General Public
 *     License as published by the Free Software Foundation; either
 *     version 2 of the License, or (at your option) any later
 *     version.
 *
 *     This program is distributed in the hope that it will be
 *     useful, but WITHOUT ANY WARRANTY; without even the implied
 *     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *     PURPOSE.  See the GNU General Public License for more
 *     details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this program; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *     Boston, MA  02110-1301, USA.
 *
 */

/*
 * Base64 handling elsewhere in mutt should be modified to call
 * these routines. These routines were written because IMAP's
 * AUTHENTICATE protocol required them, and base64 handling
 * elsewhere wasn't sufficiently generic.
 *
 */

/*
 * This code is heavily modified from fetchmail (also GPL'd, of
 * course) by Brendan Cully <brendan@kublai.com>.
 *
 * Original copyright notice:
 *
 * The code in the fetchmail distribution is Copyright 1997 by Eric
 * S. Raymond.  Portions are also copyrighted by Carl Harris, 1993
 * and 1995.  Copyright retained for the purpose of protecting free
 * redistribution of source.
 *
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "mutt.h"
#include "mime.h"

#define BAD     -1

void mutt_buffer_to_base64 (BUFFER *out, const unsigned char *in, size_t len)
{
  mutt_buffer_increase_size (out,
                             ((len * 2) > LONG_STRING) ? (len * 2) : LONG_STRING);
  mutt_to_base64 ((unsigned char *)out->data, in, len, out->dsize);
  mutt_buffer_fix_dptr (out);
}

static void to_base64 (unsigned char *out, const unsigned char *in, size_t len,
		     size_t olen, const char *dict)
{
  while (len >= 3 && olen > 4)
  {
    *out++ = dict[in[0] >> 2];
    *out++ = dict[((in[0] << 4) & 0x30) | (in[1] >> 4)];
    *out++ = dict[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
    *out++ = dict[in[2] & 0x3f];
    olen  -= 4;
    len   -= 3;
    in    += 3;
  }

  /* clean up remainder */
  if (len > 0 && olen > 4)
  {
    unsigned char fragment;

    *out++ = dict[in[0] >> 2];
    fragment = (in[0] << 4) & 0x30;
    if (len > 1)
      fragment |= in[1] >> 4;
    *out++ = dict[fragment];
    *out++ = (len < 2) ? '=' : dict[(in[1] << 2) & 0x3c];
    *out++ = '=';
  }
  *out = '\0';

}

/* raw bytes to null-terminated base 64 string */
void mutt_to_base64 (unsigned char *out, const unsigned char *in, size_t len,
		     size_t olen)
{
  to_base64 (out, in, len, olen, B64Chars);
}

void mutt_to_base64_safeurl (unsigned char *out, const unsigned char *in,
			     size_t len, size_t olen)
{
  to_base64 (out, in, len, olen, B64Chars_urlsafe);
}

int mutt_buffer_from_base64 (BUFFER *out, const char *in)
{
  int olen;

  mutt_buffer_increase_size (out, mutt_strlen (in));
  olen = mutt_from_base64 (out->data, in, out->dsize);
  /* mutt_from_base64 returns raw bytes, so don't terminate the buffer either */
  if (olen > 0)
    out->dptr = out->data + olen;
  else
    out->dptr = out->data;

  return olen;
}

/* Convert '\0'-terminated base 64 string to raw bytes.
 * Returns length of returned buffer, or -1 on error */
int mutt_from_base64 (char *out, const char *in, size_t olen)
{
  int len = 0;
  register unsigned char digit1, digit2, digit3, digit4;

  do
  {
    digit1 = in[0];
    if (digit1 > 127 || base64val (digit1) == BAD)
      return -1;
    digit2 = in[1];
    if (digit2 > 127 || base64val (digit2) == BAD)
      return -1;
    digit3 = in[2];
    if (digit3 > 127 || ((digit3 != '=') && (base64val (digit3) == BAD)))
      return -1;
    digit4 = in[3];
    if (digit4 > 127 || ((digit4 != '=') && (base64val (digit4) == BAD)))
      return -1;
    in += 4;

    /* digits are already sanity-checked */
    if (len == olen)
      return len;
    *out++ = (base64val(digit1) << 2) | (base64val(digit2) >> 4);
    len++;
    if (digit3 != '=')
    {
      if (len == olen)
        return len;
      *out++ = ((base64val(digit2) << 4) & 0xf0) | (base64val(digit3) >> 2);
      len++;
      if (digit4 != '=')
      {
        if (len == olen)
          return len;
	*out++ = ((base64val(digit3) << 6) & 0xc0) | base64val(digit4);
	len++;
      }
    }
  }
  while (*in && digit4 != '=');

  return len;
}
