/*
 * helpers.h
 *
 *  Created on: Feb 7, 2020
 *      Author: tomer
 */

#ifndef TESTS_HELPERS_H_
#define TESTS_HELPERS_H_

#include "gtest/gtest.h"

#include <ael/log.h>

#include <netinet/in.h>

#include <string>
#include <chrono>
#include <mutex>
#include <condition_variable>

using namespace std;
using namespace ael;

class CoutSink : public ael::log::Sink {
public:
	CoutSink() {}
	virtual ~CoutSink() {}

private:
	void Log(ael::log::LogLevel log_level, const std::string &msg) override;
};

class WaitCount {
public:
	WaitCount(int count, const chrono::milliseconds &wait_time) : count_(count), wait_time_(wait_time) {}
	virtual ~WaitCount() {}

	void Dec();
	bool Wait();

private:
	int count_;
	chrono::milliseconds wait_time_;
	condition_variable cond_;
	mutex mut_;
};

class Environment : public ::testing::Environment {
 public:
  ~Environment() override {}

  void SetUp() override;
  void TearDown() override;
};

int ConnectTo(const std::string &ip_addr, in_port_t port);

#endif /* TESTS_HELPERS_H_ */
