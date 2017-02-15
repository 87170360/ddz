/* -*- mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef _CHARCODING_H_
#define _CHARCODING_H_

/* iconv wraper    ///×Ö·û±àÂë
 */

extern int cc_convert(const char  *inp,
                      int          inlen,
                      char       **out,
                      int         *outlen,
                      const char  *inc,
                      const char  *outc);

#endif /* _CHARCODING_H_ */
