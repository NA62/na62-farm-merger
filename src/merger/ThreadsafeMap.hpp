/*
 * ThreadsafeMap.h
 *
 *  Created on: Jul 6, 2012
 *      Author: root
 */

#ifndef THREADSAFEMAP_H_
#define THREADSAFEMAP_H_

#include <map>

namespace na62 {
namespace merger {

template<typename key_type, typename mapped_type, typename _Compare = std::less<key_type>, typename _Alloc = std::allocator<
		std::pair<const key_type, mapped_type> > >
class ThreadsafeMap: public std::map<key_type, mapped_type, _Compare, _Alloc> {
public:
	mapped_type&
	operator[](const key_type& obj) {
		return std::map<key_type, mapped_type, _Compare, _Alloc>::operator[](obj);
	}
};

} /* namespace merger */
} /* namespace na62 */
#endif /* THREADSAFEMAP_H_ */
