/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2020 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Darek Slusarczyk <marines@php.net>                          |
  +----------------------------------------------------------------------+
*/
#ifndef XMYSQLND_UTILS_H
#define XMYSQLND_UTILS_H

#include "util/strings.h"
#include "xmysqlnd/proto_gen/mysqlx_crud.pb.h"

namespace mysqlx {

namespace drv {

void xmysqlnd_utils_decode_doc_row(zval* src, zval* dest);
void xmysqlnd_utils_decode_doc_rows(zval* src, zval* dest);

//https://en.wikipedia.org/wiki/Percent-encoding
util::string decode_pct_path(const util::string& encoded_path);

bool operator==(const google::protobuf::Message& msg_a,
				const google::protobuf::Message& msg_b);

template <typename ContainerT>
class ReverseContainerView
{
public:
	explicit ReverseContainerView(ContainerT& container)
	  : container_{container}
	{ }

	auto begin() {
		return std::rbegin(container_);
	}
	auto end() {
		return std::rend(container_);
	}

private:
	ContainerT&  container_;
};


template<typename ContainerT>
auto Reverse(ContainerT& container)
{
	return ReverseContainerView<ContainerT>(container);
}

template<typename ContainerT>
auto Reverse(const ContainerT& container)
{
	return ReverseContainerView<const ContainerT>(container);
}

} // namespace drv

} // namespace mysqlx

#endif /* XMYSQLND_UTILS_H */
