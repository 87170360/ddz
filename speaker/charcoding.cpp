#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <iconv.h>

static int
__iconv_string(const char  *in_p,
               size_t       in_len,
               char       **out,
               size_t      *out_len,
               const char  *in_charset,
               const char  *out_charset)
{
    iconv_t cd;
    size_t in_left, out_size, out_left;
    char *out_p, *out_buf, *tmp_buf;
    size_t bsz, result = 0;
    int retval = 0;

    cd = iconv_open(out_charset, in_charset);
    if (cd == (iconv_t)(-1)) {
        if (errno == EINVAL) {
            return -1; // WRONG_CHARSET0
        } else {
            return -2;
        }
    }

    in_left= in_len;
    out_left = in_len * sizeof(int) + 15; /* Avoid realloc() most cases */
    out_size = 0;
    bsz = out_left;
    out_buf = (char *)malloc(bsz+1);
    out_p = out_buf;

    while (in_left > 0)
    {
#ifdef WIN32
        result = iconv(cd, (const char **) &in_p, &in_left, (char **) &out_p, &out_left);
#else
        result = iconv(cd, (char **) &in_p, &in_left, (char **) &out_p, &out_left);
#endif
        out_size = bsz - out_left;
        if (result == (size_t)(-1)) {
            if (errno == E2BIG && in_left > 0) {
                /* converted string is longer than out buffer */
                bsz += in_len;
                tmp_buf = (char*)realloc(out_buf, bsz+1);
                if (tmp_buf != NULL) {
                    out_p = out_buf = tmp_buf;
                    out_p += out_size;
                    out_left = bsz - out_size;
                    continue;
                }
            }
        }
        break;
    }
    if (result != (size_t)(-1)) {
        /* flush the shift-out sequences */
        for (;;) {
            result = iconv(cd, NULL, NULL, (char **) &out_p, &out_left);
            out_size = bsz - out_left;
            if (result != (size_t)(-1)) {
                break;
            }
            if (errno == E2BIG) {
                bsz += 16;
                tmp_buf = (char *)realloc(out_buf, bsz);
                if (tmp_buf == NULL) {
                    break;
                }
                out_p = out_buf = tmp_buf;
                out_p += out_size;
                out_left = bsz - out_size;
            } else {
                break;
            }
        }
    }
    iconv_close(cd);
    if (result == (size_t)(-1)) {
        switch (errno) {
        case EINVAL:
            retval = -3;
            break;
        case EILSEQ:
            retval = -4;
            break;
        case E2BIG:
            retval = -5;
            break;
        default:
            retval = -6;
            break;
        }

        // failed
        *out = NULL;
        *out_len = 0;

        free(out_buf);
    }
    else {
        *out_p = '\0';
        *out = out_buf;
        *out_len = out_size;
    }

    return retval;
}

int cc_convert(const char  *inp,
               int          inlen,
               char       **out,
               int         *outlen,
               const char  *inc,
               const char  *outc)
{
    int    r;
    size_t ol;

    if (inlen <= 0)
        return -7;

    r = __iconv_string(inp, (size_t) inlen, out, &ol, inc, outc);

    *outlen = (int) ol;

    return r;
}
