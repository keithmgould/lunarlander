#include "simulator.hpp"

#include <utility>
#include <algorithm>
#include <cstdlib>
#include <cmath>

#include <Eigen/Geometry>


rigid_body::rigid_body(double mass, double mom_inertia, double mu_s, double mu_k, double restitution,
                       const std::vector<collider>& colliders)
  : mass(mass), mom_inertia(mom_inertia), mu_s(mu_s), mu_k(mu_k), restitution(restitution),
    colliders(colliders), breakage(0), bounding_radius(0) {

  pos.setZero(); vel.setZero(); rot = 0; rot_vel = 0;

  for (unsigned int i = 0; i < colliders.size(); ++i) {
    bounding_radius = std::max (bounding_radius, colliders[i].pos.norm());
  }
}


bool rigid_body::process_collisions (const double restitution, const Vector2d& new_pos, const double new_rot,
                                     const std::vector<Vector2d>& colliders_dpos_drot, const bool contact_phase) {

  if (new_pos.y() > bounding_radius) return false;

  Eigen::Rotation2Dd rot_matrix (new_rot);

  std::vector<Vector2d> colliders_rel_pos (colliders.size());
  for (unsigned int i = 0; i < colliders.size(); ++i) {
    colliders_rel_pos[i] = rot_matrix * colliders[i].pos;
  }

  std::vector<std::pair<double, size_t> > colliders_sort_ix (colliders.size());
  for (unsigned int i = 0; i < colliders.size(); ++i) {
    colliders_sort_ix[i].first = colliders_rel_pos[i].y();
    colliders_sort_ix[i].second = i;
  }

  std::sort (colliders_sort_ix.begin(), colliders_sort_ix.end());

  bool collisions = false;

  for (unsigned int sorted_index = 0; sorted_index < colliders.size(); ++sorted_index) {

    const int i = colliders_sort_ix[sorted_index].second;

    if (new_pos.y() + colliders_rel_pos[i].y() > 0) break;

    const Vector2d collider_vel = vel + rot_vel*colliders_dpos_drot[i];
    if (collider_vel.y() > 0) continue;

    Matrix2d K;
    {
      Vector2d tangential_vel (colliders_rel_pos[i].y(), -colliders_rel_pos[i].x());
      K = tangential_vel * tangential_vel.transpose();
      K /= mom_inertia;
      K += Matrix2d::Identity() / mass;
    }

    Vector2d impulse = K.llt().solve (Vector2d (-collider_vel.x(), -(1+restitution)*collider_vel.y()));

    if (std::abs (impulse.x()) > mu_s*impulse.y()) {
      const double friction = collider_vel.x() > 0 ? -mu_k : mu_k;
      impulse.y() = -(1+restitution) * collider_vel.y() / (friction*K(1,0) + K(1,1));
      impulse.x() = impulse.y() * friction;
    }

    apply_impulse (colliders_rel_pos[i], impulse);
    (contact_phase ? colliders[i].contacted : colliders[i].collided) = true;
    collisions = true;

    colliders[i].impulse += rot_matrix.inverse() * impulse;
  }

  return collisions;

}


void rigid_body::update (const double dt, const Vector2d& force, const double torque) {

  std::vector<Vector2d> colliders_dpos_drot (colliders.size());

  {
    Eigen::Rotation2Dd rot_matrix (rot);

    for (unsigned int i = 0; i < colliders.size(); ++i) {

      Vector2d collider_dpos_drot (-colliders[i].pos.y(), colliders[i].pos.x());
      colliders_dpos_drot[i] = rot_matrix * collider_dpos_drot;

      colliders[i].reset_collision();
    }
  }

  const Vector2d delta_vel = force * (dt / mass);
  const double delta_rot_vel = torque * (dt / mom_inertia);

  // Collision

  for (unsigned int i = 0; i < 5; ++i) {

    const Vector2d new_pos = pos + dt*(vel + delta_vel);
    const double new_rot = rot + dt*(rot + delta_rot_vel);

    if (!process_collisions (restitution, new_pos, new_rot, colliders_dpos_drot, false)) break;
  }

  // Velocity update

  vel += delta_vel;
  rot_vel += delta_rot_vel;

  // Contact

  for (int i = -9; i <= 0; ++i) {

    const Vector2d new_pos = pos + dt*(vel + delta_vel);
    const double new_rot = rot + dt*(rot + delta_rot_vel);

    if (!process_collisions (i/10.0, new_pos, new_rot, colliders_dpos_drot, true)) break;
  }

  // Position update

  pos += dt * vel;
  rot += dt * rot_vel;

  // Breakage

  for (unsigned int i = 0; i < colliders.size(); ++i) {
    breakage = std::max (breakage, (colliders[i].strength * colliders[i].impulse).norm());
  }

}


double rigid_body::get_min_y() {
  Eigen::Rotation2Dd rot_mat(rot);
  double min_y = 0;
  for (unsigned int i = 0; i < colliders.size(); ++i) {
    min_y = std::max(min_y, -(rot_mat * colliders[i].pos).y());
  }
  return min_y;
}

rigid_body::collider lunar_lander_simulator::MAKE_LEG_COLLIDER (double x, double y, double strut_dir) {
  const double strength = 3.0e4;
  const double shear = strength * 0.4;
  double cos_dir = std::cos(strut_dir);
  double sin_dir = std::sin(strut_dir);
  Matrix2d strength_mat;
  strength_mat <<
    cos_dir/strength, sin_dir/strength,
    -sin_dir/shear,   cos_dir/shear;
  return rigid_body::collider(IMAGE_TO_BODY_COORDS(x,y), strength_mat);
}

rigid_body::collider lunar_lander_simulator::MAKE_BODY_COLLIDER (double x, double y) {
  return rigid_body::collider(IMAGE_TO_BODY_COORDS(x,y), Matrix2d::Identity());
}

std::vector<rigid_body::collider> lunar_lander_simulator::MAKE_COLLIDERS() {
  std::vector<rigid_body::collider> colliders;
  const double pi = std::atan(1)*4;
  colliders.push_back(MAKE_LEG_COLLIDER  (0.0541, 0.0456, pi/6));
  colliders.push_back(MAKE_LEG_COLLIDER  (0.9459, 0.0456, pi*5/6));
  colliders.push_back(MAKE_LEG_COLLIDER  (0.0000, 0.0627, pi/6));
  colliders.push_back(MAKE_LEG_COLLIDER  (1.0000, 0.0626, pi*5/6));
  colliders.push_back(MAKE_BODY_COLLIDER (0.2251, 0.6980));
  colliders.push_back(MAKE_BODY_COLLIDER (0.4729, 0.8348));
  colliders.push_back(MAKE_BODY_COLLIDER (0.6211, 0.6809));
  colliders.push_back(MAKE_BODY_COLLIDER (0.7493, 0.4929));
  return colliders;
}

lunar_lander_simulator::lunar_lander_simulator () :
  lander(11036.4, // MASS in kg
         28258.7, // MOM_INERTIA in kg m^2
         1.0,     // MU_S
         0.9,     // MU_K
         0.2,     // RESTITUTION
         MAKE_COLLIDERS())
{
  initialize();
}


void lunar_lander_simulator::initialize(double pos_x, double pos_y, double vel_x, double vel_y, double rot, double rot_vel) {
  lander.set_pos(Vector2d (pos_x, pos_y));
  lander.set_rot(rot);
  lander.set_vel(Vector2d (vel_x, vel_y));
  lander.set_rot_vel(rot_vel);

  lander.reset_collisions();
  lander.reset_breakage();

  crashed = false;
  landed = false;
  current_action = action();

  update(0.0);
}


void lunar_lander_simulator::update(double dt) {

  const double GRAVITY = 1.622; // m/s^2

  Vector2d accel = Eigen::Rotation2Dd (lander.get_rot()) * Vector2d (0, current_action.thrust);
  accel.y() -= GRAVITY;
  lander.update(dt, accel*lander.get_mass(), current_action.rcs*lander.get_mom_inertia());

  if (!crashed) {
    if (lander.get_breakage() > 1) {
      crashed = true;
    }
    else if (lander.get_colliders()[0].contacted && lander.get_colliders()[1].contacted) {
      double ground_vel = std::abs(lander.get_vel()(0));
      if (ground_vel > 1) crashed = true;
      else if (ground_vel < 0.5) landed = true;
    }
  }
}

void lunar_lander_simulator::set_action(const action& new_action) {
  current_action.thrust = std::max (0.0, std::min(MAX_THRUST(), new_action.thrust));
  current_action.rcs = std::max (-MAX_RCS(), std::min(MAX_RCS(), new_action.rcs));
}
