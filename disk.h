/*
 * disk.h -- public interface to disk scheduler output functions.
 *
 */
#pragma once

void print_request(unsigned int requester, unsigned int track);
void print_service(unsigned int requester, unsigned int track);
