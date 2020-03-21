/*
  ==============================================================================

    BufferedLagrange.h
    Created: 10 Jan 2018 4:42:07pm
    Author:  IVerhage

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <boost/circular_buffer.hpp>

#include <string>
#include <vector>

class BufferedLagrange
	: public LagrangeInterpolator {

private:
	std::vector<float> buf;

public:
	int getFramesToWork(double srcSr, double destSr, int destNumberOfFrames);

	void resample(std::vector<float>* srcData, double srcSr, float* destData, int destDataSize, double destSr);
	void resample(const float* srcData, int srcSize, double srcSr, float* destData, int destDataSize, double destSr);

	void setName(std::string name) {
		this->name = name;
	}

private:
	int proc(double ratio, std::vector<float>* in, float* out, int numSamplesOut);
	std::string name;

public:
	BufferedLagrange();
	~BufferedLagrange();

};
