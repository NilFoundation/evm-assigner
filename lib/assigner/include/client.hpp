//---------------------------------------------------------------------------//
// Copyright (c) Nil Foundation and its affiliates.
//
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.
//---------------------------------------------------------------------------//

#ifndef EVM1_ASSIGNER_INCLUDE_CLIENT_HPP_
#define EVM1_ASSIGNER_INCLUDE_CLIENT_HPP_

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

int GetBalance()
{

    std::cout << "RUN GetBalance\n";
    auto const host = "127.0.0.1";//"http://127.0.0.1";
    auto const port = "8529";
    auto const target = "";
    int version = 10;

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    std::cout << "SSL CLIENT\n";
    httplib::SSLClient cli("127.0.0.1", 8529);
    //cli.set_ca_cert_path(CA_CERT_FILE);
    cli.enable_server_certificate_verification(false);
#else
    std::cout << "NO SSL CLIENT\n";
    httplib::Client cli("localhost", 8529);
#endif

    if (auto res = cli.Get("/hi")) {
        std::cout << res->status << "\n";
        std::cout << res->get_header_value("Content-Type") << "\n";
        std::cout << res->body << "\n";
    } else {
        std::cout << "error code: " << res.error() << "\n";
    }

    return 0;
}

#endif    // EVM1_ASSIGNER_INCLUDE_CLIENT_HPP_
