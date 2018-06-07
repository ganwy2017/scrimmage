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

#include <scrimmage/entity/Contact.h>
#include <scrimmage/math/State.h>
#include <scrimmage/parse/ParseUtils.h>
#include <scrimmage/plugin_manager/RegisterPlugin.h>
#include <scrimmage/proto/ExternalControl.pb.h>

#include <scrimmage/plugins/autonomy/RLSimple/RLSimple.h>

#include <iostream>

#include <boost/range/algorithm/count_if.hpp>

namespace sp = scrimmage_proto;
namespace br = boost::range;

REGISTER_PLUGIN(scrimmage::Autonomy, scrimmage::autonomy::RLSimple, RLSimple_plugin)

namespace scrimmage {
namespace autonomy {

void RLSimple::init(std::map<std::string, std::string> &params) {
    x_discrete_ = str2bool(params.at("x_discrete"));
    y_discrete_ = str2bool(params.at("y_discrete"));
    ctrl_y_ = str2bool(params.at("ctrl_y"));

    using Type = VariableIO::Type;
    using Dir = VariableIO::Direction;

    output_vel_x_idx_ = vars_.declare(Type::velocity_x, Dir::Out);
    output_vel_y_idx_ = vars_.declare(Type::velocity_y, Dir::Out);
    uint8_t output_vel_z_idx = vars_.declare(Type::velocity_z, Dir::Out);

    vars_.output(output_vel_x_idx_, 0);
    vars_.output(output_vel_y_idx_, 0);
    vars_.output(output_vel_z_idx, 0);

    radius_ = std::stod(params.at("radius"));

    ExternalControl::init(params);

    if (x_discrete_) {
        action_space.discrete_maxima.push_back(1);
    } else {
        const double inf = std::numeric_limits<double>::infinity();
        action_space.continuous_extrema.push_back(std::make_pair(-inf, inf));
    }

    if (ctrl_y_) {
        if (y_discrete_) {
            action_space.discrete_maxima.push_back(1);
        } else {
            const double inf = std::numeric_limits<double>::infinity();
            action_space.continuous_extrema.push_back(std::make_pair(-inf, inf));
        }
    }
}

std::pair<bool, double> RLSimple::calc_reward(double t, double dt) {
    const bool done = false;
    const double x = state_->pos()(0);
    const bool within_radius = std::round(std::abs(x)) < radius_;
    double reward = within_radius ? 1 : 0;
    return {done, reward};
}

double RLSimple::action_getter(bool discrete, int idx) {
    if (discrete) {
        return action_.discrete(idx) ? 1 : -1;
    } else {
        return action_.continuous(idx);
    }
}

bool RLSimple::step_autonomy(double /*t*/, double /*dt*/) {
    if (action_.done()) return false;

    int num_discrete = x_discrete_ + (ctrl_y_ && y_discrete_);
    int num_continuous = !x_discrete_ + (ctrl_y_ && !y_discrete_);

    if (!check_action(action_, num_discrete, num_continuous)) return false;

    double x_vel = action_getter(x_discrete_, 0);
    double y_vel = ctrl_y_ ? action_getter(y_discrete_, 1) : 0;

    vars_.output(output_vel_x_idx_, x_vel);
    vars_.output(output_vel_y_idx_, y_vel);
    return true;
}

} // namespace autonomy
} // namespace scrimmage
