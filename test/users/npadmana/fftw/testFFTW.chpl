use FFTW;


proc printcmp(x, y) {
  var err = max reduce abs(x-y);
  //writeln("----");
  //writeln(x);
  //writeln("**");
  //writeln(y);
  //writeln("----");
  writeln(err);
}

proc runtest(param ndim : int, fn : string) {
  /* Read in the data and set up the domains appropriately */
  var dims : ndim*int(32); 
  var D : domain(ndim);
  // Define ranges here
  var rD,cD,reD,imD : domain(ndim,int,true); 
  var A,B,goodA,goodB : [D] complex;
  {
    var f = open(fn,iomode.r).reader(kind=iokind.little);
    for ii in 1..ndim {
      f.read(dims(ii));
    }
    // Note compiler does not automatically fold select on params
    if (ndim == 1) {
      D = 0.. #dims(ndim);
    }
    if (ndim == 2) {
      D = {0.. #dims(1),0.. #dims(ndim)};
    }
    if (ndim == 3) {
      D = {0.. #dims(1),0.. #dims(2), 0.. #dims(ndim)};
    }
    for val in goodA {
      f.read(val.re);
      val.im = 0;
    }
    for val in goodB {
      f.read(val);
    }
    f.close();
    writeln("Data read...");
  }
  // Set up domains based on dimension size
  if (ndim==1) {
    var ldim = dims(1)/2 + 1;
    // Domains for real FFT
    rD = 0.. #(2*ldim); // Padding to do in place transforms
    cD = 0.. #ldim;
    // Define domains to extract the real and imaginary parts for inplace transforms
    reD = rD[0..(2*ldim-1) by 2]; // Padding to do in place transforms
    imD = rD[1..(2*ldim-1) by 2]; // Padding to do in place transforms
  }
  if (ndim==2) {
    // Domains for real FFT
    var ldim = dims(2)/2+1;
    rD = {0.. #dims(1),0.. #(2*ldim)}; // Padding to do in place transforms
    cD = {0.. #dims(1),0.. #ldim};
    // Define domains to extract the real and imaginary parts for in place transforms
    reD = rD[..,0..(2*ldim-1) by 2]; // Padding to do in place transforms
    imD = rD[..,1..(2*ldim-1) by 2]; // Padding to do in place transforms
  }
  if (ndim==3) {
    // Domains for real FFT
    var ldim = dims(3)/2+1;
    rD = {0.. #dims(1),0.. #dims(2),0.. #(2*ldim)}; // Padding to do in place transforms
    cD = {0.. #dims(1),0.. #dims(2),0.. #ldim};
    // Define domains to extract the real and imaginary parts for in place transforms
    reD = rD[..,..,0..(2*ldim-1) by 2]; // Padding to do in place transforms
    imD = rD[..,..,1..(2*ldim-1) by 2]; // Padding to do in place transforms
  }

  // FFTW does not normalize inverse transform, set up norm
  var norm = * reduce dims;

	// FFT testing here
	var fwd = plan_dft(A, B, FFTW_FORWARD, FFTW_ESTIMATE);
	var rev = plan_dft(B, A, FFTW_BACKWARD, FFTW_ESTIMATE);
	// Test forward and reverse transform
	A = goodA;
	execute(fwd);
	printcmp(B,goodB);
	execute(rev);
	A /= norm; 
	printcmp(A,goodA);
	destroy_plan(fwd);
	destroy_plan(rev);

	// Test in-place transforms
	fwd = plan_dft(A, FFTW_FORWARD, FFTW_ESTIMATE);
	rev = plan_dft(A, FFTW_BACKWARD, FFTW_ESTIMATE);
	A = goodA;
	// Test forward and reverse transform
	A = goodA;
	execute(fwd);
	printcmp(A,goodB);
	execute(rev);
	A /= norm; // FFTW does an unnormalized transform
	printcmp(A,goodA);
	destroy_plan(fwd);
	destroy_plan(rev);

  // Testing r2c and c2r
  var rA : [D] real(64); // No padding for an out-of place transform
  var cB : [cD] complex;
  fwd = plan_dft_r2c(rA,cB,FFTW_ESTIMATE);
  rev = plan_dft_c2r(cB,rA,FFTW_ESTIMATE);
  rA[D] = goodA.re;
  execute(fwd);
  printcmp(cB,goodB[cD]);
  execute(rev);
  rA /= norm;
  printcmp(rA[D],goodA.re);
  destroy_plan(fwd);
  destroy_plan(rev);
  // In place transform
  var rA2 : [rD] real(64);
  fwd = plan_dft_r2c(D,rA2,FFTW_ESTIMATE);
  rev = plan_dft_c2r(D,rA2,FFTW_ESTIMATE);
  rA2[D] = goodA.re;
  execute(fwd);
  printcmp(rA2[reD],goodB[cD].re);
  printcmp(rA2[imD],goodB[cD].im);
  execute(rev);
  rA2 /= norm;
  printcmp(rA2[D],goodA.re);
  destroy_plan(fwd);
  destroy_plan(rev);

}


proc testAllDims() {
  writeln("1D");
  runtest(1, "arr1d.dat");
  writeln("2D");
  runtest(2, "arr2d.dat");
  writeln("3D");
  runtest(3, "arr3d.dat");
}

proc main() {
  testAllDims();
  cleanup();
}
