#ifndef SOFTDSP_PRIMITIVE_HPP
#define SOFTDSP_PRIMITIVE_HPP

#include <boost/mpl/or.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>

namespace softdsp {
  template< typename T >
  struct is_primitive
  : public boost::mpl::or_<
    boost::is_integral<
      typename boost::remove_cv<
        T
      >::type
    >,
    boost::is_floating_point<
      typename boost::remove_cv<
        T
      >::type
    >
  >{};
}

#endif
