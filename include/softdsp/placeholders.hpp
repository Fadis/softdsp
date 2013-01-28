#ifndef SOFTDSP_PLACEHOLDERS_HPP
#define SOFTDSP_PLACEHOLDERS_HPP

#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/inc.hpp>
#include <boost/proto/proto.hpp>

namespace softdsp {
  namespace proto = boost::proto;
  using proto::argsns_::list2;
  using proto::exprns_::expr;
  struct placeholder_ {};
  template < typename Index >
  struct placeholder : public placeholder_ {};
#define SOFTDSP_PLACEHOLDER( z, index, unused ) \
  boost::proto::terminal< placeholder< boost::mpl::int_< index > > >::type const BOOST_PP_CAT( _, BOOST_PP_INC( index ) ) = {{}};
  BOOST_PP_REPEAT( FUSION_MAX_VECTOR_SIZE, SOFTDSP_PLACEHOLDER, )
}

#endif
