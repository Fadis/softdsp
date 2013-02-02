#ifndef SOFTDSP_MPL_HPP
#define SOFTDSP_MPL_HPP

#include <boost/mpl/at.hpp>
#include <boost/none.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/support/is_sequence.hpp>

namespace softdsp {
  namespace mpl {
    template< typename T, typename Index, bool is_sequence = boost::enable_if< boost::fusion::traits::is_sequence< T > >::value >
    struct at {};
    template< typename T, typename Index >
    struct at< T, Index, true > : public boost::mpl::at< T, Index > {};
    template< typename T, typename Index >
    struct at< T, Index, false > {
      typedef boost::none_t type;
    };
  }
}

#endif
