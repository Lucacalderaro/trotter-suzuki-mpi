/**
 * Massively Parallel Trotter-Suzuki Solver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <fstream>
#include "trottersuzuki.h"
#include "common.h"

void calculate_borders(int coord, int dim, int * start, int *end, int *inner_start, int *inner_end, int length, int halo, int periodic_bound) {
    int inner = (int)ceil((double)length / (double)dim);
    *inner_start = coord * inner;
    if(periodic_bound != 0)
        *start = *inner_start - halo;
    else
        *start = ( coord == 0 ? 0 : *inner_start - halo );
    *end = *inner_start + (inner + halo);

    if (*end > length) {
        if(periodic_bound != 0)
            *end = length + halo;
        else
            *end = length;
    }
    if(periodic_bound != 0)
        *inner_end = *end - halo;
    else
        *inner_end = ( *end == length ? *end : *end - halo );
}

double const_potential(double x, double y) {
    return 0.;
}


Lattice::Lattice(int dim, double _length_x, double _length_y, int _periods[2],
                 double omega): length_x(_length_x), length_y(_length_y) {
    delta_x = length_x / double(dim);
    delta_y = length_y / double(dim);
    if (_periods == 0) {
          periods[0] = 0;
          periods[1] = 0;
    } else {
          periods[0] = _periods[0];
          periods[1] = _periods[1];
    }
    mpi_dims[0] = mpi_dims[1] = 0;
#ifdef HAVE_MPI
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_procs);
    MPI_Dims_create(mpi_procs, 2, mpi_dims);  //partition all the processes (the size of MPI_COMM_WORLD's group) into an 2-dimensional topology
    MPI_Cart_create(MPI_COMM_WORLD, 2, mpi_dims, periods, 0, &cartcomm);
    MPI_Comm_rank(cartcomm, &mpi_rank);
    MPI_Cart_coords(cartcomm, mpi_rank, 2, mpi_coords);
#else
    mpi_procs = 1;
    mpi_rank = 0;
    mpi_dims[0] = mpi_dims[1] = 1;
    mpi_coords[0] = mpi_coords[1] = 0;
#endif
    halo_x = (omega == 0. ? 4 : 8);
    halo_y = (omega == 0. ? 4 : 8);
    global_dim_x = dim + periods[1] * 2 * halo_x;
    global_dim_y = dim + periods[0] * 2 * halo_y;
    //set dimension of tiles and offsets
    calculate_borders(mpi_coords[1], mpi_dims[1], &start_x, &end_x,
                      &inner_start_x, &inner_end_x,
                      dim, halo_x, periods[1]);
    calculate_borders(mpi_coords[0], mpi_dims[0], &start_y, &end_y,
                      &inner_start_y, &inner_end_y,
                      dim, halo_y, periods[0]);
    dim_x = end_x - start_x;
    dim_y = end_y - start_y;
}


State::State(Lattice *_grid, double *_p_real, double *_p_imag): grid(_grid){
    if (_p_real == 0) {
        self_init = true;
        p_real = new double[grid->dim_x * grid->dim_y];
    } else {
        self_init = false;
        p_real = _p_real;
    }
    if (_p_imag == 0) {
        p_imag = new double[grid->dim_x * grid->dim_y];
    } else {
        p_imag = _p_imag;
    }
}

State::~State() {
    if (self_init) {
        delete p_real;
        delete p_imag;
    }
}

void State::init_state(complex<double> (*ini_state)(double x, double y)) {
    complex<double> tmp;
    double delta_x = grid->delta_x, delta_y = grid->delta_y;
    double idy = grid->start_y * delta_y, idx;
    for (int y = 0; y < grid->dim_y; y++, idy += delta_y) {
        idx = grid->start_x * delta_x;
        for (int x = 0; x < grid->dim_x; x++, idx += delta_x) {
            tmp = ini_state(idx, idy);
            p_real[y * grid->dim_x + x] = real(tmp);
            p_imag[y * grid->dim_x + x] = imag(tmp);
        }
    }
}


void State::read_state(char *file_name, int read_offset) {
    ifstream input(file_name);
    int in_width = grid->global_dim_x - 2 * grid->periods[1] * grid->halo_x;
    int in_height = grid->global_dim_y - 2 * grid->periods[0] * grid->halo_y;
    complex<double> tmp;
    for(int i = 0; i < read_offset; i++)
        input >> tmp;
    for(int i = 0; i < in_height; i++) {
        for(int j = 0; j < in_width; j++) {
            input >> tmp;
            if((i - grid->start_y) >= 0 && (i - grid->start_y) < grid->dim_y && (j - grid->start_x) >= 0 && (j - grid->start_x) < grid->dim_x) {
                p_real[(i - grid->start_y) * grid->dim_x + j - grid->start_x] = real(tmp);
                p_imag[(i - grid->start_y) * grid->dim_x + j - grid->start_x] = imag(tmp);
            }

            //Down band
            if(i < grid->halo_y && grid->mpi_coords[0] == grid->mpi_dims[0] - 1 && grid->periods[0] != 0) {
                if((j - grid->start_x) >= 0 && (j - grid->start_x) < grid->dim_x) {
                    p_real[(i + grid->dim_y - grid->halo_y) * grid->dim_x + j - grid->start_x] = real(tmp);
                    p_imag[(i + grid->dim_y - grid->halo_y) * grid->dim_x + j - grid->start_x] = imag(tmp);
                }
                //Down right corner
                if(j < grid->halo_x && grid->periods[1] != 0 && grid->mpi_coords[1] == grid->mpi_dims[1] - 1) {
                    p_real[(i + grid->dim_y - grid->halo_y) * grid->dim_x + j + grid->dim_x - grid->halo_x] = real(tmp);
                    p_imag[(i + grid->dim_y - grid->halo_y) * grid->dim_x + j + grid->dim_x - grid->halo_x] = imag(tmp);
                }
                //Down left corner
                if(j >= in_width - grid->halo_x && grid->periods[1] != 0 && grid->mpi_coords[1] == 0) {
                    p_real[(i + grid->dim_y - grid->halo_y) * grid->dim_x + j - (in_width - grid->halo_x)] = real(tmp);
                    p_imag[(i + grid->dim_y - grid->halo_y) * grid->dim_x + j - (in_width - grid->halo_x)] = imag(tmp);
                }
            }

            //Upper band
            if(i >= in_height - grid->halo_y && grid->periods[0] != 0 && grid->mpi_coords[0] == 0) {
                if((j - grid->start_x) >= 0 && (j - grid->start_x) < grid->dim_x) {
                    p_real[(i - (in_height - grid->halo_y)) * grid->dim_x + j - grid->start_x] = real(tmp);
                    p_imag[(i - (in_height - grid->halo_y)) * grid->dim_x + j - grid->start_x] = imag(tmp);
                }
                //Up right corner
                if(j < grid->halo_x && grid->periods[1] != 0 && grid->mpi_coords[1] == grid->mpi_dims[1] - 1) {
                    p_real[(i - (in_height - grid->halo_y)) * grid->dim_x + j + grid->dim_x - grid->halo_x] = real(tmp);
                    p_imag[(i - (in_height - grid->halo_y)) * grid->dim_x + j + grid->dim_x - grid->halo_x] = imag(tmp);
                }
                //Up left corner
                if(j >= in_width - grid->halo_x && grid->periods[1] != 0 && grid->mpi_coords[1] == 0) {
                    p_real[(i - (in_height - grid->halo_y)) * grid->dim_x + j - (in_width - grid->halo_x)] = real(tmp);
                    p_imag[(i - (in_height - grid->halo_y)) * grid->dim_x + j - (in_width - grid->halo_x)] = imag(tmp);
                }
            }

            //Right band
            if(j < grid->halo_x && grid->periods[1] != 0 && grid->mpi_coords[1] == grid->mpi_dims[1] - 1) {
                if((i - grid->start_y) >= 0 && (i - grid->start_y) < grid->dim_y) {
                    p_real[(i - grid->start_y) * grid->dim_x + j + grid->dim_x - grid->halo_x] = real(tmp);
                    p_imag[(i - grid->start_y) * grid->dim_x + j + grid->dim_x - grid->halo_x] = imag(tmp);
                }
            }

            //Left band
            if(j >= in_width - grid->halo_x && grid->periods[1] != 0 && grid->mpi_coords[1] == 0) {
                if((i - grid->start_y) >= 0 && (i - grid->start_y) < grid->dim_y) {
                    p_real[(i - grid->start_y) * grid->dim_x + j - (in_width - grid->halo_x)] = real(tmp);
                    p_imag[(i - grid->start_y) * grid->dim_x + j - (in_width - grid->halo_x)] = imag(tmp);
                }
            }
        }
    }
    input.close();
}

double State::calculate_squared_norm(bool global) {
    double norm2 = 0;
    int tile_width = grid->end_x - grid->start_x;
#ifndef HAVE_MPI
        #pragma omp parallel for reduction(+:norm2)
#endif    
    for(int i = grid->inner_start_y - grid->start_y; i < grid->inner_end_y - grid->start_y; i++) {
        for(int j = grid->inner_start_x - grid->start_x; j < grid->inner_end_x - grid->start_x; j++) {
          norm2 += p_real[j + i * tile_width] * p_real[j + i * tile_width] + p_imag[j + i * tile_width] * p_imag[j + i * tile_width];
        }
    }
#ifdef HAVE_MPI
    if (global) {
        double *sums = new double[grid->mpi_procs];
        MPI_Allgather(&norm2, 1, MPI_DOUBLE, sums, 1, MPI_DOUBLE, grid->cartcomm);
        norm2 = 0.;
        for(int i = 0; i < grid->mpi_procs; i++)
            norm2 += sums[i];
        delete [] sums;
    }
#endif
    return norm2 * grid->delta_x * grid->delta_y;
}

double *State::get_particle_density(double *_density) {
    double *density;
    if (_density == 0) {
      density = new double[grid->dim_x * grid->dim_y];
    } else {
      density = _density;
    }
    for(int j = grid->inner_start_y - grid->start_y; j < grid->inner_end_y - grid->start_y; j++) {
        for(int i = grid->inner_start_x - grid->start_x; i < grid->inner_end_x - grid->start_x; i++) {
            density[j * grid->dim_x + i] = p_real[j * grid->dim_x + i] * p_real[j * grid->dim_x + i] + p_imag[j * grid->dim_x + i] * p_imag[j * grid->dim_x + i];
      }
    }
    return density;
}

void State::write_particle_density(string fileprefix) {
    double *density = new double[grid->dim_x * grid->dim_y];
    stringstream filename;
    filename << fileprefix << "-density";
    stamp_matrix(grid, get_particle_density(density), filename.str());
    delete density;
}

double *State::get_phase(double *_phase) {
    double *phase;
    if (_phase == 0) {
      phase = new double[grid->dim_x * grid->dim_y];
    } else {
      phase = _phase;
    }
    double norm;
    for(int j = grid->inner_start_y - grid->start_y; j < grid->inner_end_y - grid->start_y; j++) {
        for(int i = grid->inner_start_x - grid->start_x; i < grid->inner_end_x - grid->start_x; i++) {
            norm = sqrt(p_real[j * grid->dim_x + i] * p_real[j * grid->dim_x + i] + p_imag[j * grid->dim_x + i] * p_imag[j * grid->dim_x + i]);
            if(norm == 0)
                phase[j * grid->dim_x + i] = 0;
            else
                phase[j * grid->dim_x + i] = acos(p_real[j * grid->dim_x + i] / norm) * ((p_imag[j * grid->dim_x + i] > 0) - (p_imag[j * grid->dim_x + i] < 0));
        }
    }
    return phase;
}

void State::write_phase(string fileprefix) {
    double *phase = new double[grid->dim_x * grid->dim_y];
    stringstream filename;
    filename << fileprefix << "-phase";
    stamp_matrix(grid, get_phase(phase), filename.str());
    delete phase;
}

void State::calculate_mean_position(int grid_origin_x, int grid_origin_y,
                                    double *results, double norm2) {

    int ini_halo_x = grid->inner_start_x - grid->start_x;
    int ini_halo_y = grid->inner_start_y - grid->start_y;
    int end_halo_x = grid->end_x - grid->inner_end_x;
    int end_halo_y = grid->end_y - grid->inner_end_y;
    int tile_width = grid->end_x - grid->start_x;

    if(norm2 == 0)
        norm2 = calculate_squared_norm();

    complex<double> sum_x_mean = 0, sum_xx_mean = 0, sum_y_mean = 0, sum_yy_mean = 0;
    complex<double> psi_center;
    for (int i = grid->inner_start_y - grid->start_y + (ini_halo_y == 0);
         i < grid->inner_end_y - grid->start_y - (end_halo_y == 0); i++) {
        for (int j = grid->inner_start_x - grid->start_x + (ini_halo_x == 0);
             j < grid->inner_end_x - grid->start_x - (end_halo_x == 0); j++) {
            psi_center = complex<double> (p_real[i * tile_width + j], p_imag[i * tile_width + j]);
            sum_x_mean += conj(psi_center) * psi_center * complex<double>(grid->delta_x * (j - grid_origin_x), 0.);
            sum_y_mean += conj(psi_center) * psi_center * complex<double>(grid->delta_y * (i - grid_origin_y), 0.);
            sum_xx_mean += conj(psi_center) * psi_center * complex<double>(grid->delta_x * (j - grid_origin_x), 0.) * complex<double>(grid->delta_x * (j - grid_origin_x), 0.);
            sum_yy_mean += conj(psi_center) * psi_center * complex<double>(grid->delta_y * (i - grid_origin_y), 0.) * complex<double>(grid->delta_y * (i - grid_origin_y), 0.);
        }
    }

    results[0] = real(sum_x_mean / norm2) * grid->delta_x * grid->delta_y;
    results[2] = real(sum_y_mean / norm2) * grid->delta_x * grid->delta_y;
    results[1] = real(sum_xx_mean / norm2) * grid->delta_x * grid->delta_y - results[0] * results[0];
    results[3] = real(sum_yy_mean / norm2) * grid->delta_x * grid->delta_y - results[2] * results[2];
}

void State::calculate_mean_momentum(double *results, double norm2) {

    int ini_halo_x = grid->inner_start_x - grid->start_x;
    int ini_halo_y = grid->inner_start_y - grid->start_y;
    int end_halo_x = grid->end_x - grid->inner_end_x;
    int end_halo_y = grid->end_y - grid->inner_end_y;
    int tile_width = grid->end_x - grid->start_x;

    if(norm2 == 0)
        norm2 = calculate_squared_norm();

    complex<double> sum_px_mean = 0, sum_pxpx_mean = 0, sum_py_mean = 0,
                         sum_pypy_mean = 0,
                         var_px = complex<double>(0., - 0.5 / grid->delta_x),
                         var_py = complex<double>(0., - 0.5 / grid->delta_y);
    complex<double> psi_up, psi_down, psi_center, psi_left, psi_right;
    for (int i = grid->inner_start_y - grid->start_y + (ini_halo_y == 0);
         i < grid->inner_end_y - grid->start_y - (end_halo_y == 0); i++) {
        for (int j = grid->inner_start_x - grid->start_x + (ini_halo_x == 0);
             j < grid->inner_end_x - grid->start_x - (end_halo_x == 0); j++) {
            psi_center = complex<double> (p_real[i * tile_width + j],
                                               p_imag[i * tile_width + j]);
            psi_up = complex<double> (p_real[(i - 1) * tile_width + j],
                                           p_imag[(i - 1) * tile_width + j]);
            psi_down = complex<double> (p_real[(i + 1) * tile_width + j],
                                             p_imag[(i + 1) * tile_width + j]);
            psi_right = complex<double> (p_real[i * tile_width + j + 1],
                                              p_imag[i * tile_width + j + 1]);
            psi_left = complex<double> (p_real[i * tile_width + j - 1],
                                             p_imag[i * tile_width + j - 1]);

            sum_px_mean += conj(psi_center) * (psi_right - psi_left);
            sum_py_mean += conj(psi_center) * (psi_up - psi_down);
            sum_pxpx_mean += conj(psi_center) * (psi_right - 2. * psi_center + psi_left);
            sum_pypy_mean += conj(psi_center) * (psi_up - 2. * psi_center + psi_down);
        }
    }

    sum_px_mean = sum_px_mean * var_px;
    sum_py_mean = sum_py_mean * var_py;
    sum_pxpx_mean = sum_pxpx_mean * (-1.)/(grid->delta_x * grid->delta_x);
    sum_pypy_mean = sum_pypy_mean * (-1.)/(grid->delta_y * grid->delta_y);

    results[0] = real(sum_px_mean / norm2) * grid->delta_x * grid->delta_y;
    results[2] = real(sum_py_mean / norm2) * grid->delta_x * grid->delta_y;
    results[1] = real(sum_pxpx_mean / norm2) * grid->delta_x * grid->delta_y - results[0] * results[0];
    results[3] = real(sum_pypy_mean / norm2) * grid->delta_x * grid->delta_y - results[2] * results[2];
}

void State::write_to_file(string filename) {
    stamp(grid, this, filename);
}

ExponentialState::ExponentialState(Lattice *_grid, int _n_x, int _n_y, double _norm, double _phase, double *_p_real, double *_p_imag): 
                  State(_grid, _p_real, _p_imag), n_x(_n_x), n_y(_n_y), norm(_norm), phase(_phase) {
    complex<double> tmp;
    double delta_x = grid->delta_x, delta_y = grid->delta_y;
    double idy = grid->start_y * delta_y, idx;
    for (int y = 0; y < grid->dim_y; y++, idy += delta_y) {
        idx = grid->start_x * delta_x;
        for (int x = 0; x < grid->dim_x; x++, idx += delta_x) {
            tmp = exp_state(idx, idy);
            p_real[y * grid->dim_x + x] = real(tmp);
            p_imag[y * grid->dim_x + x] = imag(tmp);
        }
    }
}

complex<double> ExponentialState::exp_state(double x, double y) {
	double L_x = (grid->global_dim_x - 2.*grid->halo_x * grid->periods[1]) * grid->delta_x;
    double L_y = (grid->global_dim_y - 2.*grid->halo_x * grid->periods[0]) * grid->delta_y;
    return sqrt(norm/(L_x*L_y)) * exp(complex<double>(0., phase)) * exp(complex<double>(0., 2*M_PI*double(n_x)/L_x * x + 2*M_PI* double(n_y)/L_y * y));
}

GaussianState::GaussianState(Lattice *_grid, double _omega, double _mean_x, double _mean_y, 
                             double _norm, double _phase, double *_p_real, double *_p_imag): 
                             State(_grid, _p_real, _p_imag), mean_x(_mean_x),
                             mean_y(_mean_y), omega(_omega), norm(_norm), phase(_phase) {
    complex<double> tmp;
    double delta_x = grid->delta_x, delta_y = grid->delta_y;
    double idy = grid->start_y * delta_y, idx;
    for (int y = 0; y < grid->dim_y; y++, idy += delta_y) {
        idx = grid->start_x * delta_x;
        for (int x = 0; x < grid->dim_x; x++, idx += delta_x) {
            tmp = gauss_state(idx, idy);
            p_real[y * grid->dim_x + x] = real(tmp);
            p_imag[y * grid->dim_x + x] = imag(tmp);
        }
    }
}   

complex<double> GaussianState::gauss_state(double x, double y) {
    double x_c = (grid->global_dim_x - 2.*grid->halo_x * grid->periods[1]) * grid->delta_x * 0.5;
    double y_c = (grid->global_dim_y - 2.*grid->halo_y * grid->periods[1]) * grid->delta_y * 0.5;
    return complex<double>(sqrt(norm * omega / M_PI) * exp(-(pow(x + mean_x - x_c, 2.0) + pow(y + mean_y - y_c, 2.0)) * 0.5 * omega), 0.) * exp(complex<double>(0., phase));  
}

SinusoidState::SinusoidState(Lattice *_grid, int _n_x, int _n_y, double _norm, double _phase, double *_p_real, double *_p_imag): 
                             State(_grid, _p_real, _p_imag), n_x(_n_x), n_y(_n_y), norm(_norm), phase(_phase)  {
    complex<double> tmp;
    double delta_x = grid->delta_x, delta_y = grid->delta_y;
    double idy = grid->start_y * delta_y, idx;
    for (int y = 0; y < grid->dim_y; y++, idy += delta_y) {
        idx = grid->start_x * delta_x;
        for (int x = 0; x < grid->dim_x; x++, idx += delta_x) {
            tmp = sinusoid_state(idx, idy);
            p_real[y * grid->dim_x + x] = real(tmp);
            p_imag[y * grid->dim_x + x] = imag(tmp);
        }
    }
}

complex<double> SinusoidState::sinusoid_state(double x, double y) {
    double L_x = (grid->global_dim_x - 2.*grid->halo_x * grid->periods[1]) * grid->delta_x;
    double L_y = (grid->global_dim_y - 2.*grid->halo_x * grid->periods[0]) * grid->delta_y;
    return sqrt(norm/(L_x*L_y)) * 2.* exp(complex<double>(0., phase)) * complex<double> (sin(2*M_PI*double(n_x) / L_x*x) * sin(2*M_PI*double(n_y) / L_y*y), 0.0);
}

Potential::Potential(Lattice *_grid, char *filename): grid(_grid) {
    matrix = new double[grid->dim_y * grid->dim_x];
    self_init = true;
    is_static = true;
    ifstream input(filename);
    double tmp;
    for(int y = 0; y < grid->dim_y; y++) {
        for(int x = 0; x < grid->dim_x; x++) {
            input >> tmp;
            matrix[y * grid->dim_y + x] = tmp;
        }
    }
    input.close();
}

Potential::Potential(Lattice *_grid, double *_external_pot): grid(_grid) {
    matrix = _external_pot;
    self_init = false;
    is_static = true;
    evolving_potential = NULL;
    static_potential = NULL;
}

Potential::Potential(Lattice *_grid, double (*potential_fuction)(double x, double y)): grid(_grid) {
    is_static = true;
    self_init = false;
    evolving_potential = NULL;
    static_potential = potential_fuction;
    matrix = NULL;
}

Potential::Potential(Lattice *_grid, double (*potential_function)(double x, double y, double t), int _t): grid(_grid) {
    is_static = false;
    self_init = false;    
    evolving_potential = potential_function;
    static_potential = NULL;
    matrix = NULL;
}

double Potential::get_value(int x, int y) {
    if (matrix != NULL) {
        return matrix[y * grid->dim_x + x];
    } else {
        double idy = grid->start_y * grid->delta_y + y*grid->delta_y;
        double idx = grid->start_x * grid->delta_x + x*grid->delta_x;
        if (is_static) {
            static_potential(idx, idy);
        } else {
            evolving_potential(idx, idy, current_evolution_time);
        }
    }
}

bool Potential::update(double t) {
    current_evolution_time = t;
    if (!is_static) {
        if (matrix != NULL) {
            double delta_x = grid->delta_x, delta_y = grid->delta_y;
            double idy = grid->start_y * delta_y, idx;
            for (int y = 0; y < grid->dim_y; y++, idy += delta_y) {
                idx = grid->start_x * delta_x;
                for (int x = 0; x < grid->dim_x; x++, idx += delta_x) {
                    matrix[y * grid->dim_x + x] = evolving_potential(idx, idy, t);
                }
            }
        }
        return true;
    } else {
        return false;
    }
}

Potential::~Potential() {
    if (self_init) {
        delete [] matrix;
    }
}

ParabolicPotential::ParabolicPotential(Lattice *_grid, double _omegax, double _omegay, double _mass, double _mean_x, double _mean_y): 
                                       Potential(_grid, const_potential), omegax(_omegax), omegay(_omegay), 
                                       mass(_mass), mean_x(_mean_x), mean_y(_mean_y) {
    is_static = true;
    self_init = false;
    evolving_potential = NULL;
    static_potential = NULL;
    matrix = NULL;
}

double ParabolicPotential::get_value(int x, int y) {
    double idy = (grid->start_y + y) * grid->delta_y + mean_x;
    double idx = (grid->start_x + x) * grid->delta_x + mean_y;
    double x_c = (grid->global_dim_x - 2.*grid->halo_x * grid->periods[1]) * grid->delta_x * 0.5;
    double y_c = (grid->global_dim_y - 2.*grid->halo_y * grid->periods[1]) * grid->delta_y * 0.5;
    double x_r = idx - x_c;
    double y_r = idy - y_c;
    return 0.5 * mass * (omegax * omegax * x_r * x_r + omegay * omegay * y_r * y_r);
}

ParabolicPotential::~ParabolicPotential() {
}

Hamiltonian::Hamiltonian(Lattice *_grid, Potential *_potential,
                         double _mass, double _coupling_a,
                         double _angular_velocity,
                         double _rot_coord_x, double _rot_coord_y): grid(_grid), mass(_mass),
                         coupling_a(_coupling_a), angular_velocity(_angular_velocity) {
    rot_coord_x = (grid->global_dim_x - grid->periods[1] * 2 * grid->halo_x) * 0.5 + _rot_coord_x / grid->delta_x;
    rot_coord_y = (grid->global_dim_y - grid->periods[1] * 2 * grid->halo_y) * 0.5 + _rot_coord_y / grid->delta_y;
    if (_potential == NULL) {
        self_init = true;
        potential = new Potential(grid, const_potential);
    } else {
        self_init = false;
        potential = _potential;
    }
}

Hamiltonian::~Hamiltonian() {
    if (self_init) {
        delete potential;
    }
}

Hamiltonian2Component::Hamiltonian2Component(Lattice *_grid,
                         Potential *_potential,
                         Potential *_potential_b,                         
                         double _mass,
                         double _mass_b, double _coupling_a,
                         double _coupling_ab, double _coupling_b,
                         double _omega_r, double _omega_i,
                         double _angular_velocity,
                         double _rot_coord_x, double _rot_coord_y):
                         Hamiltonian(_grid, _potential, _mass, _coupling_a, _angular_velocity, _rot_coord_x, rot_coord_y), mass_b(_mass_b),
                         coupling_ab( _coupling_ab), coupling_b(_coupling_b), omega_r(_omega_r), omega_i(_omega_i) {

    if (_potential_b == NULL) {
        potential_b = _potential;
    } else {
        potential_b = _potential_b;
    }
}

Hamiltonian2Component::~Hamiltonian2Component() {

}
