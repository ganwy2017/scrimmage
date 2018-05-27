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

#include <grpc++/grpc++.h>

#include <scrimmage/common/Time.h>
#include <scrimmage/common/Utilities.h>
#include <scrimmage/entity/Entity.h>
#include <scrimmage/parse/MissionParse.h>
#include <scrimmage/plugin_manager/RegisterPlugin.h>
#include <scrimmage/plugins/autonomy/ExternalControl/ExternalControl.h>
#include <scrimmage/plugins/autonomy/ExternalControl/ExternalControlClient.h>
#include <scrimmage/plugins/interaction/ExternalControlInteraction/ExternalControlInteraction.h>
#include <scrimmage/math/State.h>

#include <memory>
#include <limits>
#include <iostream>

#include <boost/optional.hpp>

namespace sp = scrimmage_proto;

REGISTER_PLUGIN(scrimmage::EntityInteraction,
                scrimmage::interaction::ExternalControlInteraction,
                ExternalControlInteraction_plugin)

namespace scrimmage {
namespace interaction {

bool ExternalControlInteraction::init(std::map<std::string, std::string> &mission_params,
                                      std::map<std::string, std::string> &plugin_params) {
    std::string server_address = plugin_params.at("server_address");
    delayed_task_.delay = std::stod(plugin_params.at("timestep"));
    external_control_client_ = std::make_shared<autonomy::ExternalControlClient>();
    *external_control_client_ =
        autonomy::ExternalControlClient(grpc::CreateChannel(
            server_address, grpc::InsecureChannelCredentials()));
    external_control_client_->set_timeout(std::stod(plugin_params.at("timeout")));

    print_err_on_exit = false;
    return true;
}

bool ExternalControlInteraction::step_entity_interaction(
        std::list<EntityPtr> &ents, double t, double dt) {

    if (ents.empty()) return true;

    if (ext_ctrl_vec_.empty()) {
        for (auto &e : ents) {
            for (auto &a : e->autonomies()) {
                auto p = std::dynamic_pointer_cast<autonomy::ExternalControl>(a);
                if (p) {
                    ext_ctrl_vec_.push_back(p);
                    action_results_.add_action_results();
                }
            }
        }
    }

    if (ext_ctrl_vec_.empty()) return true;

    // receive action from grpc, send action to each autonomy, get reward/obs
    if (!env_sent_) {
        if (!send_env()) return false;
        env_sent_ = true;
    }

    // get reward/obs/done
    // terminate is supposed to occur after the first state is experienced
    update_reward(t, dt);
    if (!delayed_task_.update(t).first) {
        if (action_results_.done()) {
            external_control_client_->send_action_results(action_results_);
            return false;
        } else {
            return true;
        }
    } else {

        update_observation(t);
        auto actions = external_control_client_->send_action_results(action_results_);

        bool done;
        if (actions && actions->actions_size() == static_cast<int>(ext_ctrl_vec_.size())) {
            for (int i = 0; i < actions->actions_size(); i++) {
                ext_ctrl_vec_[i]->set_action(actions->actions(i));
            }
            done = false;
        } else {
            sp::Action action;
            action.set_done(true);
            for (auto ext_ctrl : ext_ctrl_vec_) {
                ext_ctrl->set_action(action);
            }
            done = true;
        }

        done |= action_results_.done() || !actions || actions->done();

        // reset action_results
        for (size_t i = 0; i < ext_ctrl_vec_.size(); i++) {
            action_results_.mutable_action_results(i)->set_reward(0);
        }
        return !done;
    }
}

void ExternalControlInteraction::update_reward(double t, double dt) {

    bool done = false;

    for (size_t i = 0; i < ext_ctrl_vec_.size(); i++) {
        double reward;
        bool temp_done;
        std::tie(temp_done, reward) = ext_ctrl_vec_[i]->calc_reward(t, dt);

        done |= temp_done;
        auto *ar = action_results_.mutable_action_results(i);
        ar->set_done(temp_done);
        ar->set_reward(ar->reward() + reward);
    }

    action_results_.set_done(done);
}

void ExternalControlInteraction::update_observation(double t) {
    for (size_t i = 0; i < ext_ctrl_vec_.size(); i++) {
        ext_ctrl_vec_[i]->get_observation(
            t, action_results_.mutable_action_results(i)->mutable_observations());
    }
}

bool ExternalControlInteraction::send_env() {
    sp::Environments envs;
    for (auto &a : ext_ctrl_vec_) {
        *envs.add_envs() = a->get_env();
    }

    return external_control_client_->send_environments(envs);
}

void ExternalControlInteraction::close(double t) {
    update_observation(t);
    action_results_.set_done(true);
    external_control_client_->send_action_results(action_results_);
}
} // namespace interaction
} // namespace scrimmage
