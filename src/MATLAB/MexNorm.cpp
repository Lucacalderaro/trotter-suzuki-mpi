#include "mex.h"
#include "trottersuzuki.h"

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[]) {
    
    int nCol = mxGetN(prhs[0]);
    int nRow = mxGetM(prhs[0]);
    
    //check inputs           
    if(nrhs != 4) {
		mexErrMsgIdAndTxt("MexTrotter:"," four inputs required.");
	}
	
	if( !mxIsDouble(prhs[0]) || mxIsComplex(prhs[0])) {
		mexErrMsgIdAndTxt("MexTrotter:"," input 1: notDouble Input, matrix must be type double.");
	}
	
	if( !mxIsDouble(prhs[1]) || mxIsComplex(prhs[1])) {
		mexErrMsgIdAndTxt("MexTrotter:"," input 2: notDouble Input, matrix must be type double.");
	}
	
	if( !mxIsDouble(prhs[2]) || mxIsComplex(prhs[2]) || mxGetNumberOfElements(prhs[2]) != 1 ) {
		mexErrMsgIdAndTxt("MexTrotter:"," input 3: notScalar Input, delta_x must be a scalar.");
	}
	if( !mxIsDouble(prhs[3]) || mxIsComplex(prhs[3]) || mxGetNumberOfElements(prhs[3]) != 1 ) {
		mexErrMsgIdAndTxt("MexTrotter:"," input 4: notScalar Input, delta_y must be a scalar.");
	}
	
	//check matrices dimensions    
    if (nCol != mxGetN(prhs[1])) {
        mexErrMsgIdAndTxt("MexTrotter:", "Input 2 must have the same number of columns as input 1");
    }
    if (nRow != mxGetM(prhs[1])) {
        mexErrMsgIdAndTxt("MexTrotter:", "Input 2 must have the same number of rows as input 1");
    }
    
	//check outputs
	if(nlhs != 1) {
		mexErrMsgIdAndTxt("MexTrotter:"," one output required.");
	}
	
    // initialize matrices
    double *Mp_real = mxGetPr(prhs[0]);
    double *p_real = new double[nCol * nRow];
    double *Mp_imag = mxGetPr(prhs[1]);
    double *p_imag = new double[nCol * nRow];
            
    for (int i = 0; i < nRow; i++) {
        for (int j = 0; j < nCol; j++) {
            p_real[i * nCol + j] = (double)Mp_real[j * nRow + i];
            p_imag[i * nCol + j] = (double)Mp_imag[j * nRow + i];
        }
    }
    
    //initialize scalars
    double delta_x = (double)mxGetPr(prhs[2])[0];
    double delta_y = (double)mxGetPr(prhs[3])[0];
    
    //launch the function
    double Norm_2 = Norm2(p_real, p_imag, nCol, nRow, delta_x, delta_y);
    
    //Set output
    plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);
    double* OutNorm_2 = mxGetPr(plhs[0]);
    OutNorm_2[0] = Norm_2;
}
