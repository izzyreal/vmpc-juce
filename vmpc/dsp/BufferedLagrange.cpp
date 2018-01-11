/*
  ==============================================================================

    BufferedLagrange.cpp
    Created: 10 Jan 2018 4:42:07pm
    Author:  IVerhage

  ==============================================================================
*/

#include "BufferedLagrange.h"

BufferedLagrange::BufferedLagrange() {
}

void BufferedLagrange::resample(std::vector<float>* srcData, double srcSr, float* destData, int destDataSize, double destSr) {
	if (srcSr == destSr) return;
	//MLOG("\nbuf size: " + std::to_string(buf.size()));
	if (!buf.size() == 0) {
		//MLOG("srcData size before: " + std::to_string(srcData->size()));
		srcData->insert(srcData->begin(), buf.begin(), buf.end());
		//MLOG("srcData size after: " + std::to_string(srcData->size()));
		buf.clear();
	}

	auto ratio = srcSr / destSr;
	//MLOG("input samples provided: " + std::to_string(srcData->size()));
	auto res = proc(ratio, srcData, destData, destDataSize);
	//MLOG("input samples used: " + std::to_string(res));

	if (res != srcData->size()) {
		int diff = srcData->size() - res;
		int counter = srcData->size() - diff;
		for (int i = 0; i < diff; i++) {
			buf.push_back((*srcData)[counter++]);
		}
	}
}

int BufferedLagrange::getFramesToWork(double srcSr, double destSr, int destNumberOfFrames) {
	auto ratio = srcSr / destSr;
	auto res = (int)(ceil)(destNumberOfFrames * ratio) - buf.size();
	return res;
}

int BufferedLagrange::proc(double ratio, std::vector<float>* in, float* out, int numSamplesOut) {
	const float* inArray = &(*in)[0];
	return process(ratio, inArray, out, numSamplesOut);
}


BufferedLagrange::~BufferedLagrange() {
}
