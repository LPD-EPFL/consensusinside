/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 */

#ifndef MPASS_TLS_H_
#define MPASS_TLS_H_

namespace mpass {

#ifdef USE_PTHREAD_TLS

#include <pthread.h>

namespace mpass {
	/**
	 * This is a TLS class that will put one instance of templated
	 * class into TLS storage and provide access to it. Assumption here
	 * is that the TLS class exposes default constructor. If this is
	 * not the case this class should be slightly changed.
	 * 
	 */
	template<class T>
	class Tls {
		public:
			static T *Get();

		private:
			static ::pthread_key_t tlsKey;
	};
}

template<class T> ::pthread_key_t mpass::Tls<T>::tlsKey;

template<class T>
inline T *mpass::Tls<T>::Get() {
	return (T *)::pthread_getspecific(tlsKey);
}

#else

namespace mpass {

	template<class T>
	class Tls {
		public:
			static T *Get() {
				return val;
			}

		private:
			static __thread T *val;
	};
}

#if defined(__INTEL_COMPILER)
template<class T> T* mpass::Tls<T>::val;
#else
template<class T> __thread T* mpass::Tls<T>::val;
#endif


#endif // USE_PTHREAD_TLS

#endif // MPASS_TLS_H_
