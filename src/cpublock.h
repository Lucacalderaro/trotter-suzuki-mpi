/**
 * Distributed Trotter-Suzuki solver
 * Copyright (C) 2015 Luca Calderaro, 2012-2015 Peter Wittek,
 * 2010-2012 Carlos Bederián
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
 
 //added delta_x, delta_y, coupling_const

#ifndef __CPUBLOCK_H
#define __CPUBLOCK_H

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include "kernel.h"
#ifdef HAVE_MPI
#include <mpi.h>
#endif

#define BLOCK_WIDTH 128u
#define BLOCK_HEIGHT 128u

//Helpers
void block_kernel_vertical(size_t start_offset, size_t stride, size_t width, size_t height, double a, double b, double * p_real, double * p_imag);
void block_kernel_horizontal(size_t start_offset, size_t stride, size_t width, size_t height, double a, double b, double * p_real, double * p_imag);

void block_kernel_vertical_imaginary(size_t start_offset, size_t stride, size_t width, size_t height, double a, double b, double * p_real, double * p_imag);
void block_kernel_horizontal_imaginary(size_t start_offset, size_t stride, size_t width, size_t height, double a, double b, double * p_real, double * p_imag);

class CPUBlock: public ITrotterKernel {
public:
    CPUBlock(double **_p_real, double **_p_imag, double **_external_pot_real, double **_external_pot_imag, double _a, double _b, double *_coupling_const, double _delta_x, double _delta_y, 
             int matrix_width, int matrix_height, int _halo_x, int _halo_y, int *_periods, double _norm, bool _imag_time, double _alpha_x, double _alpha_y, int _rot_coord_x, int _rot_coord_y
#ifdef HAVE_MPI
             , MPI_Comm cartcomm
#endif
            );
    ~CPUBlock();
    void run_kernel();
    void run_kernel_on_halo();
    void wait_for_completion(int iteration);
    void get_sample(size_t dest_stride, size_t x, size_t y, size_t width, size_t height, double ** dest_real, double ** dest_imag) const;
    void rabi_coupling(double var, double delta_t);
    void normalization();

    bool runs_in_place() const {
        return false;
    }
    std::string get_name() const {
        return "CPU";
    };

    void start_halo_exchange();
    void finish_halo_exchange();



private:
    void kernel8(const double *p_real, const double *p_imag, const double *pb_real, const double *pb_imag, double * next_real, double * next_imag);
    double *p_real[2][2];
    double *p_imag[2][2];
    double *external_pot_real[2];
    double *external_pot_imag[2];
    double a;
    double b;
    double delta_x, delta_y;
    double norm;
    double *coupling_const;
    int sense, state;
    size_t halo_x, halo_y, tile_width, tile_height;
    bool imag_time;
    static const size_t block_width = BLOCK_WIDTH;
    static const size_t block_height = BLOCK_HEIGHT;
	
	double alpha_x, alpha_y;
	int rot_coord_x, rot_coord_y;
    int start_x, inner_end_x, start_y, inner_start_y, inner_end_y;
    int inner_start_x, end_x, end_y;
    int *periods;
#ifdef HAVE_MPI
    MPI_Comm cartcomm;
    int neighbors[4];
    MPI_Request req[8];
    MPI_Status statuses[8];
    MPI_Datatype horizontalBorder, verticalBorder;
#endif
};

#endif
