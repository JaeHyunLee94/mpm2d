//
// Created by test on 2021-10-15.
//

#ifndef MPM2D_SIMHEADER_H
#define MPM2D_SIMHEADER_H

typedef Eigen::Vector2d Vec2;
typedef Eigen::Vector3d Vec3;
typedef Eigen::Matrix2d Mat2;
typedef double Scalar;

struct GridNode {
    Vec2 m_vel_i;
    Vec2 m_force_i;
    Scalar m_mass_i;
};
struct Particle {
    /*
     *  Note that we only need to store
     *  1. Elastic deformation gradient F_e
     *  2  Plastic deformation determinant J_p
     */
    Vec2 m_pos_p;
    Vec2 m_vel_p;
    Mat2 m_vel_grad;
    Scalar m_mass_p;
    Mat2 m_F;   //Elastic deformation gradient
    Mat2 m_Ap;  //for computation Vp x Cauchy stress
    Scalar m_J_p;   // Plastic deformation determinant


};
int grid_size;
Scalar dt;
Vec2 gravity;
Scalar V0;
Scalar radius;
Scalar particle_mass;
Scalar dx;
Scalar inv_dx;
Scalar boundary;
Vec2 center1, center2;
Scalar E, nu, mu0, lambda0, hardening, critical_comp, critical_stretch, rho_0;

unsigned int particle_num;
unsigned int particle_num_per_lump; //particle per one snow lump
std::vector<Particle> particles;
std::vector<std::vector<GridNode>> grid;


void simulationInit() {
    /*
     * initialize simulation parameters.
     *
     * */

    dt = 0.00005;                                   //timestep
    grid_size = 256;
    particle_num_per_lump = 2048;
    particle_num = 2 * particle_num_per_lump;
    radius = 0.05;                                  //snow radius (left one)
    gravity = Vec2{0, -9.89};
    V0 = 1;                                         //initial volume
    particles.resize(particle_num);
    particle_mass = 1.0;
    dx = 1. / grid_size;
    inv_dx = 1. / dx;
    center1 = Vec2(0.2, 0.5);                   //snow center (left one)
    center2 = Vec2(0.7, 0.6);                   //snow center  (right one)
    boundary = 0.01;                                  //grid boundary


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, 9999);

    hardening = 10;                                                     //snow hardening Ksi
    E = 1.4e7;                                                           //Young's modulus
    nu = 0.2;
    mu0 = E / (2 * (1 + nu));                                           //lame's 1 coeff
    lambda0 = E * nu / ((1 + nu) * (1 - 2 * nu));                       //lame's 2 coeff
    printf("mu0: %f, lamda0: %f\n", mu0, lambda0);

    critical_comp = 1.5e-2;                                             //theta c
    critical_stretch =3.5e-3;                                           //theta s
    //particle init
    for (int i = 0; i < particle_num_per_lump; i++) {
        int random_num = dis(gen);
        Scalar r = (radius / 10000.0) * (Scalar) dis(gen);
        Scalar theta = (360.0 / 10000.0) * (Scalar) dis(gen);
        particles[i].m_pos_p(0) = center1(0) + r * cos(theta);
        particles[i].m_pos_p(1) = center1(1) + r * sin(theta);
        particles[i].m_vel_p.setZero();
        particles[i].m_vel_p(0) =10;
        particles[i].m_vel_grad.setZero();

        particles[i].m_F.setIdentity();
        particles[i].m_J_p = 1;
        particles[i].m_mass_p = particle_mass;
        particles[i].m_Ap.setZero();

    }
    for (int i = particle_num_per_lump; i < particle_num; i++) {
        int random_num = dis(gen);
        Scalar r = (1.5 * radius / 10000.0) * (Scalar) dis(gen);
        Scalar theta = (360.0 / 10000.0) * (Scalar) dis(gen);
        particles[i].m_pos_p(0) = center2(0) + r * cos(theta);
        particles[i].m_pos_p(1) = center2(1) + r * sin(theta);
        particles[i].m_vel_p.setZero();
        particles[i].m_vel_p(0) = -10;
        particles[i].m_vel_grad.setZero();

        particles[i].m_F.setIdentity();
        particles[i].m_J_p = 1;
        particles[i].m_mass_p = particle_mass;
        particles[i].m_Ap.setZero();

    }

    //grid init
    grid.resize(grid_size);
    for (auto &g: grid) {
        g.resize(grid_size);
        for (int i = 0; i < grid_size; i++) {
            //vel + mass
            g[i].m_vel_i.setZero();
            g[i].m_force_i.setZero();
            g[i].m_mass_i = 0;
        }
    }


};
Scalar N(Scalar x) {
    /*
     * Cubic B-spline function
     */
    Scalar W;
    x = fabs(x);

    if (x < 1)
        W = (x * x * x / 2.0 - x * x + 2 / 3.0);

    else if (x < 2)
        W = (2 - x) * (2 - x) * (2 - x) / 6.0;

    else
        W = 0;

    return W;
}
Scalar diff_N(Scalar x) {
    /*
    * Differentiation of Cubic B-spline
    */

    Scalar dW;
    Scalar x_abs;
    x_abs = fabs(x);

    if (x_abs < 1)
        dW = 1.5 * x * x_abs - 2.0 * x;

    else if (x_abs < 2)
        dW = -x * x_abs / 2.0 + 2 * x - 2 * x / x_abs;

    else
        dW = 0;

    return dW;


}
Scalar W(Vec2 dist) {


    /*
     * Weight
     */
    return N(dist(0)) * N(dist(1));

}
Vec2 grad_W(Vec2 dist) {

    /*
     * Gradient of Weight
     */
    return Vec2(
            diff_N(dist[0]) * N(dist[1]),
            N(dist[0]) * diff_N(dist[1]));

}
Vec2 clamp(Vec2 &vec, Scalar minimum, Scalar maximum) {

    /*
     * Clamping Sigama of SVD
     */
    Vec2 ret;
    ret << std::clamp(vec(0), minimum, maximum), std::clamp(vec(1), minimum, maximum);
    return ret;
}
void computeAp(Particle &p) {

    /*
     * compute Vp x Cauchy stress
     */

    //compute snow plastic parameter mu, lambda
    Scalar mu = mu0 * std::exp(hardening * (1 - p.m_J_p));
    Scalar lambda = lambda0 * std::exp(hardening * (1 - p.m_J_p));
    Mat2 I;
    I.setIdentity();
    Eigen::JacobiSVD<Mat2> svd(p.m_F, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Mat2 U = svd.matrixU();
    Mat2 V = svd.matrixV();
    Mat2 Re = U * V.transpose();
    Scalar J_e = p.m_F.determinant();
    p.m_Ap = V0 * (2 * mu * (p.m_F - Re) * p.m_F.transpose() + lambda * (J_e - 1) * J_e * I);


}
void updateDeformationGradient(Particle &p) {

    /*
     * update rule of deformation gradient
     * 1. All the deformation is attributed to Elastic F
     * 2. Project(clamp) the deformation gradient of Elastic F and update plastic determinant J_p
     *
     */
    p.m_F = p.m_F + dt * p.m_vel_grad * p.m_F;
    Scalar oldJ = p.m_F.determinant();
    Eigen::JacobiSVD<Mat2> svd(p.m_F, Eigen::ComputeFullU | Eigen::ComputeFullV);
    Mat2 U = svd.matrixU();
    Mat2 V = svd.matrixV();
    Eigen::Vector2d sigma = svd.singularValues();
    Eigen::Vector2d clamped_sigma = clamp(sigma, 1 - critical_comp, 1 + critical_stretch);


    p.m_F = U * clamped_sigma.asDiagonal() * V.transpose();
    Scalar newJP = std::clamp(p.m_J_p * oldJ / p.m_F.determinant(), 0.6, 20.0);
    p.m_J_p = newJP;
}
void initGrid() {
    /*
     * Reset grid every iter
     */
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {

            grid[i][j].m_vel_i.setZero();
            grid[i][j].m_mass_i = 0;
            grid[i][j].m_force_i.setZero();

        }
    }

};
void p2g() {

    /*
     * Particle to Grid Transfer
     */
    for (auto &p : particles) {


        //nearest left-bottom grid index
        int base_x = static_cast<int> (std::floor(p.m_pos_p(0) * inv_dx));
        int base_y = static_cast<int> (std::floor(p.m_pos_p(1) * inv_dx));


        //Pre-compute Vp x Cauchy stress
        computeAp(p);

        for (int i = -1; i < 3; i++) {
            for (int j = -1; j < 3; j++) {

                int coord_x = base_x + i;
                int coord_y = base_y + j;


                //check boundary
                if (coord_x < 0 || coord_y < 0 || coord_x >= grid_size || coord_y >= grid_size) continue;
                Vec2 coord;
                coord << coord_x, coord_y;
                Vec2 dist = (p.m_pos_p - coord * dx) * inv_dx;

                Scalar Weight = W(dist);
                Vec2 dWeight = grad_W(dist);
                grid[coord_x][coord_y].m_mass_i += p.m_mass_p * Weight;
                grid[coord_x][coord_y].m_vel_i += p.m_vel_p * (p.m_mass_p * Weight);
                grid[coord_x][coord_y].m_force_i -= p.m_Ap * dWeight;


            }
        }

    }

};
void updateGridVel() {


    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (grid[i][j].m_mass_i > 0) {
                //Momentum to vel
                grid[i][j].m_vel_i /= grid[i][j].m_mass_i;


                grid[i][j].m_vel_i += ( grid[i][j].m_force_i / grid[i][j].m_mass_i+gravity) * dt;
            }


        }

    }


};
void gridCollision() {


    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            Scalar x = i * dx;
            Scalar y = j * dx;
            if (x < boundary || x > 1 - boundary) {
                grid[i][j].m_vel_i(0) = 0;
            }
            if (y < boundary || y > 1 - boundary) {
                grid[i][j].m_vel_i(1) = 0;
            }


        }

    }
};
void g2p() {

    /*
     * Gathering data from grid
    */
    for (auto &p : particles) {


        // set particle velocity and vel gradient to zero
        p.m_vel_p.setZero();
        p.m_vel_grad.setZero();
        int base_x = static_cast<int> (std::floor(p.m_pos_p(0) * inv_dx));
        int base_y = static_cast<int> (std::floor(p.m_pos_p(1) * inv_dx));


        for (int i = -1; i < 3; i++) {
            for (int j = -1; j < 3; j++) {

                int coord_x = base_x + i;
                int coord_y = base_y + j;

                if (coord_x < 0 || coord_y < 0 || coord_x >= grid_size || coord_y >= grid_size) continue;
                Vec2 coord;
                coord << coord_x, coord_y;
                Vec2 dist = (p.m_pos_p - coord * dx) * inv_dx;
                Scalar Weight = W(dist);
                Vec2 dWeight = grad_W(dist);

                //Only the explicit and PIC manner implemented here FLIP was blended for original paper.
                Vec2 vel_PIC = grid[coord_x][coord_y].m_vel_i * Weight;
                p.m_vel_p += vel_PIC;
                Mat2 vel_grad = grid[coord_x][coord_y].m_vel_i * dWeight.transpose();
                p.m_vel_grad += vel_grad;

            }
        }


    }

};
void updateParticle() {
    for (auto &p : particles) {
        //Advect particle
        p.m_pos_p += p.m_vel_p * dt;

        //Update Deformation Gradient
        updateDeformationGradient(p);

    }


};
void particleCollision() {
    for (auto &p : particles) {

        //explicit advection
        if (p.m_pos_p(0) < boundary) {
            //p.m_pos_p(0) = boundary;
            p.m_vel_p(0) = 0;
        }
        if (p.m_pos_p(0) > 1 - boundary) {
            //p.m_pos_p(0) = 1 - boundary;
            p.m_vel_p(0) = 0;
        }
        if (p.m_pos_p(1) < boundary) {
            //p.m_pos_p(1) = boundary;
            p.m_vel_p(1) = 0;
        }
        if (p.m_pos_p(1) > 1 - boundary) {
            //p.m_pos_p(1) = 1 - boundary;
            p.m_vel_p(1) = 0;
        }

    }

};
void step() {
    initGrid();
    p2g();
    updateGridVel();
    gridCollision();
    g2p();
    //particleCollision();
    updateParticle();
}
#endif //MPM2D_SIMHEADER_H
