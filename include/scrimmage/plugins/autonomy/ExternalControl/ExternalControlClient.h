/*!
 * @file
 *
 * @section LICENSE
 *
 * Copyright (C) 2017 by the Georgia Tech Research Institute (GTRI)
 *
 * This file is part of SCRIMMAGE.
 *
 *   SCRIMMAGE is free software: you can redistribute it and/or modify it under
 *   the terms of the GNU Lesser General Public License as published by the
 *   Free Software Foundation, either version 3 of the License, or (at your
 *   option) any later version.
 *
 *   SCRIMMAGE is distributed in the hope that it will be useful, but WITHOUT
 *   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 *   License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with SCRIMMAGE.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Kevin DeMarco <kevin.demarco@gtri.gatech.edu>
 * @author Eric Squires <eric.squires@gtri.gatech.edu>
 * @date 31 July 2017
 * @version 0.1.0
 * @brief Brief file description.
 * @section DESCRIPTION
 * A Long description goes here.
 *
 */
#ifndef INCLUDE_SCRIMMAGE_PLUGINS_AUTONOMY_EXTERNALCONTROL_EXTERNALCONTROLCLIENT_H_
#define INCLUDE_SCRIMMAGE_PLUGINS_AUTONOMY_EXTERNALCONTROL_EXTERNALCONTROLCLIENT_H_

#include <scrimmage/fwd_decl.h>
#include <scrimmage/proto/ExternalControl.pb.h>
#include <scrimmage/proto/ExternalControl.grpc.pb.h>

#include <map>
#include <string>

namespace grpc {
class Channel;
}

namespace boost {
template <class T> class optional;
}

namespace scrimmage {
namespace autonomy {
class ExternalControlClient {
 public:
    ExternalControlClient() = default;
    explicit ExternalControlClient(std::shared_ptr<grpc::Channel> channel);

    bool send_environments(scrimmage_proto::Environments &envs);

    boost::optional<scrimmage_proto::Actions>
    send_action_results(scrimmage_proto::ActionResults &action_results);
    void set_timeout(double timeout);

 protected:
    std::unique_ptr<scrimmage_proto::ExternalControl::Stub> stub_;
    double timeout_ = 60;
};
} // namespace autonomy
} // namespace scrimmage
#endif // INCLUDE_SCRIMMAGE_PLUGINS_AUTONOMY_EXTERNALCONTROL_EXTERNALCONTROLCLIENT_H_
