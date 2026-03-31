#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define COMPILE_ERROR_CODE 0
#define REG_SYS_BASE_ADDR  0x48000000

#ifndef REG_WRITE
#define REG_WRITE(_r, _v) ({\
		(*(volatile uint32_t *)(_r)) = (_v);\
	})
#endif

#ifndef REG_READ
#define REG_READ(_r) ({\
		(*(volatile uint32_t *)(_r));\
	})
#endif

#ifndef REG_GET_BIT
#define REG_GET_BIT(_r, _b) ({\
		(*(volatile uint32_t*)(_r) & (_b));\
	})
#endif

#ifndef REG_SET_BIT
#define REG_SET_BIT(_r, _b) ({\
		(*(volatile uint32_t*)(_r) |= (_b));\
	})
#endif

#ifndef REG_CLR_BIT
#define REG_CLR_BIT(_r, _b) ({\
		(*(volatile uint32_t*)(_r) &= ~(_b));\
	})
#endif

#ifndef REG_SET_BITS
#define REG_SET_BITS(_r, _b, _m) ({\
		(*(volatile uint32_t*)(_r) = (*(volatile uint32_t*)(_r) & ~(_m)) | ((_b) & (_m)));\
	})
#endif

#ifndef REG_GET_FIELD
#define REG_GET_FIELD(_r, _f) ({\
		((REG_READ(_r) >> (_f##_S)) & (_f##_V));\
	})
#endif

#ifndef REG_SET_FIELD
#define REG_SET_FIELD(_r, _f, _v) ({\
		(REG_WRITE((_r),((REG_READ(_r) & ~((_f##_V) << (_f##_S)))|(((_v) & (_f##_V))<<(_f##_S)))));\
	})
#endif

#ifndef REG_MCHAN_GET_FIELD
#define REG_MCHAN_GET_FIELD(_ch, _r, _f) ({\
		((REG_READ(_r) >> (_f##_MS(_ch))) & (_f##_V));\
	})
#endif

#ifndef REG_MCHAN_SET_FIELD
#define REG_MCHAN_SET_FIELD(_ch, _r, _f, _v) ({\
		(REG_WRITE((_r), ((REG_READ(_r) & ~((_f##_V) << (_f##_MS(_ch))))|(((_v) & (_f##_V))<<(_f##_MS(_ch))))));\
	})
#endif

#ifdef __cplusplus
}
#endif