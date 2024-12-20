#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include "thread.h"
#include "mutex.h"
#include "cv.h"
#include "cpu.h"

mutex m;
cv cv_not_empty;
cv cv_not_full;

int max_disk_queue;
int num_files;
int curr_track = 0;
int active_req = 0;
std::vector<int> active_reqs;

std::vector<std::string> files;

struct DiskRequest {
    int req_num;
    int track_num;
};

std::vector<DiskRequest> request_queue;

struct Request {
    int req_num;
    std::string file;
};

void requester(void * a) {
    Request *req = (Request *) a;
    int req_num = req->req_num;
    std::ifstream file(req->file);

    // Mark this requester as active.
    m.lock();
    active_req++;
    m.unlock();

    std::string track_line;
    while (std::getline(file, track_line)) {
        int track = std::stoi(track_line);
        m.lock();
        // Wait if the queue is "full" based on max_disk_queue.
        while ((int)request_queue.size() == std::min(max_disk_queue, active_req) || active_reqs[req_num]) {
            cv_not_full.wait(m);
        }

        // Set requester as active.
        active_reqs[req_num] = 1;

        // Insert the request at the end (no sorting here).
        DiskRequest dreq;
        dreq.req_num = req_num;
        dreq.track_num = track;
        request_queue.push_back(dreq);

        std::cout << "requester " << req_num << " track " << track << std::endl;

        // Signal that the queue is not empty.
        cv_not_empty.signal();
        m.unlock();
    }

    m.lock();
    // This requester is done producing requests.
    active_req--;
    // If the queue might now be empty and no more requests are coming from this requester,
    // `servicer` might be waiting, so signal cv_not_empty (it will check conditions).
    cv_not_empty.signal();
    m.unlock();
}

void servicer(void * a) {
    // Start all requesters.
    m.lock();
    for (int i = 0; i < num_files; i++) {
        active_reqs.push_back(0);
        Request *req = new Request();
        req->req_num = i;
        req->file = files[i];
        thread((thread_startfunc_t) requester, (void *) req);
    }
    m.unlock();

    while (true) {
        m.lock();
        // Wait until queue is full or no more active requesters.
        while (request_queue.size() < std::min(max_disk_queue, active_req)) {
            cv_not_empty.wait(m);
        }

        // If the queue is empty and no active requesters, we are done servicing.
        if (request_queue.empty() && active_req == 0) {
            m.unlock();
            break;
        }

        // Find the request with the shortest seek time.
        int best_index = 0;
        int best_distance = std::abs(curr_track - request_queue[0].track_num);
        for (int i = 1; i < (int)request_queue.size(); i++) {
            int dist = std::abs(curr_track - request_queue[i].track_num);
            if (dist < best_distance) {
                best_distance = dist;
                best_index = i;
            }
        }

        // Service this request.
        DiskRequest to_service = request_queue[best_index];
        curr_track = to_service.track_num;
        std::cout << "service requester " << to_service.req_num << " track " << to_service.track_num << std::endl;

        // Remove the serviced request from the queue.
        request_queue.erase(request_queue.begin() + best_index);

        // Set requester as inactive.
        active_reqs[to_service.req_num] = 0;

        // Signal that the queue may have space now for more requests.
        cv_not_full.broadcast();

        m.unlock();
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <max_disk_queue> <file1> <file2> ..." << std::endl;
        return 1;
    }

    max_disk_queue = std::stoi(argv[1]);
    num_files = argc - 2;
    for (int i = 2; i < argc; i++) {
        files.push_back(argv[i]);
    }

    cpu::boot((thread_startfunc_t) servicer, (void *) 0, 0);
    return 0;
}
