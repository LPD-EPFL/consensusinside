/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef MPASS_INTTYPES_H_
#define MPASS_INTTYPES_H_

#include <inttypes.h>

#ifndef PRIu64

#ifdef MPASS_32
#define PRIu64 "llu"
#elif defined MPASS_64
#define PRIu64 "lu"
#endif /* ARCH */

#endif /* PRIu64 */

#endif /* MPASS_INTTYPES_H_ */
