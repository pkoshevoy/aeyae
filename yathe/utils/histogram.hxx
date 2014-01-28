// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// File         : histogram.hxx
// Author       : Pavel Aleksandrovich Koshevoy
// Created      : 2006/04/05 12:36
// Copyright    : (C) 2006
// License      : MIT
// Description  : Helper functions for working with circular histograms.

#ifndef HISTOGRAM_HXX_
#define HISTOGRAM_HXX_

//----------------------------------------------------------------
// smoothout_orientation_histogram
//
extern void
smoothout_orientation_histogram(double * orientation,
				const unsigned int & bins,
				const unsigned int iterations = 3);

//----------------------------------------------------------------
// isolate_orientation_histogram_peaks
//
extern bool
isolate_orientation_histogram_peaks(double * histogram,
				    const unsigned int & bins,
				    const bool normalize = false);

//----------------------------------------------------------------
// calc_histogram_donations
//
extern unsigned int
calc_histogram_donations(const unsigned int & bins,
			 const double & r0,
			 const double & r1,
			 const double & r,
			 unsigned int * donation_bin,
			 double * donation);

//----------------------------------------------------------------
// calc_orientation_histogram_donations
//
extern unsigned int
calc_orientation_histogram_donations(const unsigned int & bins,
				     const double & angle,
				     unsigned int * donation_bin,
				     double * donation);

//----------------------------------------------------------------
// update_orientation_histogram
//
extern void
update_orientation_histogram(double * orientation,
			     const unsigned int & bins,
			     const double & angle,
			     const double & value);


#endif // HISTOGRAM_HXX_
