/*
    Copyright 2015-2024 Clément Gallet <clement.gallet@ens-lyon.org>

    This file is part of libTAS.

    libTAS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libTAS.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MemSection.h"
#include "MemScanner.h"
#include "MemScannerThread.h"
#include "MemAccess.h"
#include "CompareOperations.h"

#include <cstring>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <thread>

#define MEMORY_CHUNK_SIZE 1024*1024
#define OUTPUT_CHUNK_SIZE 4096
#define MAX_TYPE_SIZE 8

MemScannerThread::MemScannerThread(MemScanner& ms, int br, int er, uintptr_t ba, uintptr_t ea, off_t mo, uint64_t mem) : memscanner(ms), beg_region(br), end_region(er), beg_address(ba), end_address(ea), memory_offset(mo), memory_size(mem), error(ENOERROR)
{
    finished = false;
}

MemScannerThread::~MemScannerThread()
{
    if (!addresses_path.empty())
        std::remove(addresses_path.c_str());
    if (!values_path.empty())
        std::remove(values_path.c_str());
}

void MemScannerThread::create_output_files()
{
    /* Create the files with names from the thread tid */
    std::ostringstream ossa;
    ossa << memscanner.memscan_path << "/addresses-" << std::this_thread::get_id() << ".tmp";    
    addresses_path = ossa.str();

    std::ostringstream ossv;
    ossv << memscanner.memscan_path << "/memory-" << std::this_thread::get_id() << ".tmp";
    values_path = ossv.str();
}

void MemScannerThread::first_region_scan()
{
    create_output_files();    
    std::ofstream vfs(values_path, std::ofstream::binary);
    
    new_memory_size = 0;
    processed_memory_size = 0;
    
    /* Start searching from beg_address to end_address, which were split evenly
     * between all threads. Read memory by chunks */
    uintptr_t cur_beg_addr = beg_address;
    uintptr_t cur_end_addr;
    for (int r = beg_region; r <= end_region; r++) {
        const MemSection& ms = memscanner.memsections[r];

        /* Determine region start and end addresses */
        if (r > beg_region) cur_beg_addr = ms.addr;
    
        if (r == end_region) {
            if ((end_address <= ms.addr) || (end_address > ms.endaddr)) {
                error = EPROCESS;
                finished = true;
                std::cerr << "Wrong end address" << std::endl;
                return;
            }
            cur_end_addr = end_address;
        }
        else {
            cur_end_addr = ms.endaddr;
        }
        
        /* Write data */
        uint8_t chunk[4096];
        
        for (uintptr_t ca = cur_beg_addr; ca < cur_end_addr; ca += 4096) {
            int readValues = MemAccess::read(chunk, reinterpret_cast<void*>(ca), 4096);
            if (readValues < 0) {
                std::cerr << "Cound not read game process at address " << ca << std::endl;
            }
            vfs.write((char*)chunk, 4096);
            if (!vfs) {
                finished = true;
                error = EOUTPUT;
                return;
            }
            new_memory_size += 4096;
            processed_memory_size += 4096;
            
            if (memscanner.is_stopped) {
                finished = true;
                error = ESTOPPED;
                return;                
            }
        }
    }
    finished = true;
}

void MemScannerThread::first_address_scan()
{
    create_output_files();    
    std::ofstream afs(addresses_path, std::ofstream::binary);
    std::ofstream vfs(values_path, std::ofstream::binary);
    
    new_memory_size = 0;
    processed_memory_size = 0;

    /* Save in files by batches */
    uintptr_t batch_addresses[OUTPUT_CHUNK_SIZE];
    uint8_t batch_values[OUTPUT_CHUNK_SIZE*MAX_TYPE_SIZE];
    int batch_index = 0;
    
    /* Start searching from beg_address to end_address, which were split evenly
     * between all threads. Read memory by chunks */
    uintptr_t cur_beg_addr = beg_address;
    uintptr_t cur_end_addr;
    for (int r = beg_region; r <= end_region; r++) {
        const MemSection& ms = memscanner.memsections[r];

        /* Determine region start and end addresses */
        if (r > beg_region) cur_beg_addr = ms.addr;
    
        if (r == end_region) {
            if ((end_address <= ms.addr) || (end_address > ms.endaddr)) {
                error = EPROCESS;
                finished = true;
                std::cerr << "Wrong end address" << std::endl;
                return;
            }
            cur_end_addr = end_address;
        }
        else {
            cur_end_addr = ms.endaddr;
        }
        
        /* Write data */
        uint8_t chunk[4096+MAX_TYPE_SIZE]; // extra size for unaligned search
        
        for (uintptr_t ca = cur_beg_addr; ca < cur_end_addr; ca += 4096) {
            processed_memory_size += 4096;

            /* Compute how much extra data we need to read to account for unaligned
             * search, which does not apply for the end of the region */
            int extra_read = (4096+ca)<cur_end_addr ? memscanner.value_type_size-memscanner.alignment : 0;
            
            int readValues = MemAccess::read(chunk, reinterpret_cast<void*>(ca), 4096+extra_read);
            if (readValues < 0)
                continue;
            for (int v = 0; v < readValues-(memscanner.value_type_size-memscanner.alignment); v += memscanner.alignment) {
                if (CompareOperations::check_value(chunk+v)) {
                    batch_addresses[batch_index] = ca + v;
                    memcpy(batch_values+(batch_index*memscanner.value_type_size), chunk+v, memscanner.value_type_size);
                    batch_index++;
                    if (batch_index == OUTPUT_CHUNK_SIZE) {
                        afs.write((char*)batch_addresses, OUTPUT_CHUNK_SIZE*sizeof(uintptr_t));
                        vfs.write((char*)batch_values, OUTPUT_CHUNK_SIZE*memscanner.value_type_size);
                        if (!afs || !vfs) {
                            error = EOUTPUT;
                            finished = true;
                            return;
                        }
                        new_memory_size += OUTPUT_CHUNK_SIZE*memscanner.value_type_size;
                        batch_index = 0;
                    }
                }

                if (memscanner.is_stopped) {
                    error = ESTOPPED;
                    finished = true;
                    return;                
                }
            }
        }
    }
    
    /* Flush the remaining values on the batch */
    afs.write((char*)batch_addresses, batch_index*sizeof(uintptr_t));
    vfs.write((char*)batch_values, batch_index*memscanner.value_type_size);
    if (!afs || !vfs) {
        error = EOUTPUT;
        finished = true;
        return;
    }
    new_memory_size += batch_index*memscanner.value_type_size;
    finished = true;
}

void MemScannerThread::next_scan_from_region()
{
    create_output_files();    
    std::ofstream afs(addresses_path, std::ofstream::binary);
    std::ofstream vfs(values_path, std::ofstream::binary);
    
    new_memory_size = 0;
    processed_memory_size = 0;

    std::vector<uint8_t> new_memory;
    new_memory.resize(MEMORY_CHUNK_SIZE+memscanner.value_type_size-memscanner.alignment);

    /* If we compare from previous memory, read and process saved memory by
     * chunks and by region, because all threads access to the same file. */
    std::vector<char> old_memory;
    std::ifstream ivfs;
    if (memscanner.compare_type == CompareType::Previous) {
        old_memory.resize(MEMORY_CHUNK_SIZE+memscanner.value_type_size-memscanner.alignment);
        
        ivfs.open(memscanner.values_path, std::ofstream::binary);
        ivfs.seekg(memory_offset);
    }
    
    /* Save in files by batches */
    uintptr_t batch_addresses[OUTPUT_CHUNK_SIZE];
    uint8_t batch_values[OUTPUT_CHUNK_SIZE*MAX_TYPE_SIZE];
    int batch_index = 0;

    uintptr_t cur_beg_addr = beg_address;
    uintptr_t cur_end_addr;
    for (int r = beg_region; r <= end_region; r++) {
        const MemSection& ms = memscanner.memsections[r];

        /* Determine region start and end addresses */
        if (r > beg_region) cur_beg_addr = ms.addr;
    
        if (r == end_region) {
            if ((end_address <= ms.addr) || (end_address > ms.endaddr)) {
                error = EPROCESS;
                finished = true;
                std::cerr << "Wrong end address" << std::endl;
                return;
            }
            cur_end_addr = end_address;
        }
        else {
            cur_end_addr = ms.endaddr;
        }
        
        /* Read chunks of memory */
        while (cur_beg_addr < cur_end_addr) {
            uint64_t chunk_size = MEMORY_CHUNK_SIZE;
            uint64_t chunk_size_with_extra = chunk_size + memscanner.value_type_size-memscanner.alignment;
            if ((cur_end_addr - cur_beg_addr) < chunk_size_with_extra) {
                chunk_size = cur_end_addr - cur_beg_addr;
                chunk_size_with_extra = chunk_size;
            }
            
            processed_memory_size += chunk_size;

            if (memscanner.compare_type == CompareType::Previous) {
                /* We need to seek each time, because we may need to read data
                 * multiple times due to unaligned search */ 
                ivfs.seekg(memory_offset);
                ivfs.read(old_memory.data(), chunk_size_with_extra);
                memory_offset += chunk_size;
            }
            
            int readValues = MemAccess::read(new_memory.data(), reinterpret_cast<void*>(cur_beg_addr), chunk_size_with_extra);
            if (readValues < 0) {
                std::cerr << "Cound not read game process at address " << cur_beg_addr << std::endl;
            }
            if ((uint64_t)readValues < chunk_size_with_extra) {
                std::cerr << "Did not read enough memory at address " << cur_beg_addr << std::endl;
            }
            
            for (unsigned int v = 0; v < chunk_size_with_extra-(memscanner.value_type_size-memscanner.alignment); v += memscanner.alignment) {
                if (((memscanner.compare_type == CompareType::Previous) && 
                    CompareOperations::check_previous(&new_memory[v], &old_memory[v])) ||
                    ((memscanner.compare_type == CompareType::Value) && 
                    CompareOperations::check_value(&new_memory[v]))) {
                    batch_addresses[batch_index] = cur_beg_addr + v;
                    memcpy(batch_values+(batch_index*memscanner.value_type_size), &new_memory[v], memscanner.value_type_size);
                    batch_index++;
                    if (batch_index == OUTPUT_CHUNK_SIZE) {
                        afs.write((char*)batch_addresses, OUTPUT_CHUNK_SIZE*sizeof(uintptr_t));
                        vfs.write((char*)batch_values, OUTPUT_CHUNK_SIZE*memscanner.value_type_size);
                        if (!afs || !vfs) {
                            error = EOUTPUT;
                            finished = true;
                            return;
                        }
                        new_memory_size += OUTPUT_CHUNK_SIZE*memscanner.value_type_size;
                        batch_index = 0;
                    }
                }
                
                if (memscanner.is_stopped) {
                    error = ESTOPPED;
                    finished = true;
                    return;                
                }
            }
            
            cur_beg_addr += chunk_size;
        }
    }
    
    /* Flush the remaining values on the batch */
    afs.write((char*)batch_addresses, batch_index*sizeof(uintptr_t));
    vfs.write((char*)batch_values, batch_index*memscanner.value_type_size);
    if (!afs || !vfs) {
        error = EOUTPUT;
        finished = true;
        return;
    }
    new_memory_size += batch_index*memscanner.value_type_size;
    finished = true;
}

void MemScannerThread::next_scan_from_address()
{
    create_output_files();
    std::ofstream afs(addresses_path, std::ofstream::binary);
    std::ofstream vfs(values_path, std::ofstream::binary);
    
    new_memory_size = 0;
    processed_memory_size = 0;

    int size_ratio = sizeof(uintptr_t)/memscanner.value_type_size;

    std::vector<uint8_t> new_memory;
    new_memory.resize(4096+memscanner.value_type_size-memscanner.alignment);

    /* If we compare from previous memory, read and process saved memory by
     * chunks and by region, because all threads access to the same file. */
    uint64_t max_chunk_size = MEMORY_CHUNK_SIZE;
    if (memory_size < max_chunk_size)
        max_chunk_size = memory_size;

    std::vector<char> old_memory;

    std::ifstream ivfs;
    if (memscanner.compare_type == CompareType::Previous) {
        old_memory.resize(max_chunk_size+memscanner.value_type_size-memscanner.alignment);
        
        ivfs.open(memscanner.values_path, std::ifstream::binary);
    }

    std::vector<uintptr_t> old_addresses;
    old_addresses.resize(max_chunk_size/memscanner.value_type_size);

    std::ifstream iafs(memscanner.addresses_path, std::ifstream::binary);
    if (!iafs)
        std::cerr << "error: could not open file " << memscanner.addresses_path << std::endl;
    
    /* Save in files by batches */
    uintptr_t batch_addresses[OUTPUT_CHUNK_SIZE];
    uint8_t batch_values[OUTPUT_CHUNK_SIZE*MAX_TYPE_SIZE];
    int batch_index = 0;

    /* Read chunks of memory */
    uint64_t remaining_memory_size = memory_size;
    while (remaining_memory_size > 0) {
        uint64_t chunk_size = MEMORY_CHUNK_SIZE;
        uint64_t chunk_size_with_extra = chunk_size + memscanner.value_type_size-memscanner.alignment;
        if (remaining_memory_size < chunk_size_with_extra) {
            chunk_size = remaining_memory_size;
            chunk_size_with_extra = chunk_size;
        }
        
        if (memscanner.compare_type == CompareType::Previous) {
            /* We need to seek each time, because we may need to read data
             * multiple times due to unaligned search */ 
            ivfs.seekg(memory_offset);
            if (!ivfs)
                std::cerr << "error: could not seek to offset " << memory_offset << std::endl;
            ivfs.read(old_memory.data(), chunk_size_with_extra);
            if (!ivfs) {
                std::cerr << "error: only " << ivfs.gcount() << " could be read from " << chunk_size << std::endl;
                error = EINPUT;
                finished = true;
                return;
            }
        }
        iafs.seekg(memory_offset*size_ratio);
        if (!iafs)
            std::cerr << "error: could not seek to offset " << memory_offset*size_ratio << std::endl;

        iafs.read((char*)old_addresses.data(), chunk_size_with_extra*size_ratio);
        if (!iafs) {
            std::cerr << "error: only " << iafs.gcount() << " could be read from " << chunk_size << std::endl;
            error = EINPUT;
            finished = true;
            return;
        }
        
        memory_offset += chunk_size;
        
        int addr_beg_index = 0;
        int addr_end_index = chunk_size / memscanner.value_type_size;

        // std::cout << "Index " << addr_beg_index << " to " << addr_end_index << " for thread " << std::this_thread::get_id() << std::endl;
        // std::cout << "Chunk " << old_addresses[addr_beg_index] << " to " << old_addresses[addr_end_index-1] << " for thread " << std::this_thread::get_id() << std::endl;
        
        while (addr_beg_index < addr_end_index) {
            
            /* Look at all old addresses that are inside the same memory page.
             * From cheatengine source code comments, it is faster to load an 
             * entire memory page and look at the specific addresses than loading
             * each individual addresses (because caching), except if you only
             * need one address in the memory page.
             */
            uintptr_t beg_addr = old_addresses[addr_beg_index];
            uintptr_t beg_page = beg_addr & 0xfffffffffffff000;
            
            int addr_cur_index;
            for (addr_cur_index = addr_beg_index+1; addr_cur_index < addr_end_index; addr_cur_index++) {
                if ((old_addresses[addr_cur_index] & 0xfffffffffffff000) != beg_page)
                    break;
            }
            
            processed_memory_size += (addr_cur_index-addr_beg_index)*memscanner.value_type_size;

            int readValues;

            /* If only one address in page, load that address */
            if ((addr_cur_index-addr_beg_index) == 1) {
                readValues = MemAccess::read(new_memory.data(), reinterpret_cast<void*>(beg_addr), memscanner.value_type_size);
            }
            else {
                /* Load all values from first to last address */
                uintptr_t last_addr = old_addresses[addr_cur_index-1];
                readValues = MemAccess::read(new_memory.data(), reinterpret_cast<void*>(beg_addr), (last_addr-beg_addr)+memscanner.value_type_size);
            }
            if (readValues < 0)
                continue;
            
            for (int i = addr_beg_index; i < addr_cur_index; i++) {
                uintptr_t addr = old_addresses[i];
                int mem_index = addr-beg_addr;
                
                if (((memscanner.compare_type == CompareType::Previous) && 
                    CompareOperations::check_previous(&new_memory[mem_index], &old_memory[i*memscanner.value_type_size])) ||
                    ((memscanner.compare_type == CompareType::Value) && 
                    CompareOperations::check_value(&new_memory[mem_index]))) {
                    batch_addresses[batch_index] = addr;
                    memcpy(batch_values+(batch_index*memscanner.value_type_size), &new_memory[mem_index], memscanner.value_type_size);
                    batch_index++;
                    if (batch_index == OUTPUT_CHUNK_SIZE) {
                        afs.write((char*)batch_addresses, OUTPUT_CHUNK_SIZE*sizeof(uintptr_t));
                        vfs.write((char*)batch_values, OUTPUT_CHUNK_SIZE*memscanner.value_type_size);
                        if (!afs || !vfs) {
                            error = EOUTPUT;
                            finished = true;
                            return;
                        }
                        new_memory_size += OUTPUT_CHUNK_SIZE*memscanner.value_type_size;
                        batch_index = 0;
                    }
                }
                
                if (memscanner.is_stopped) {
                    error = ESTOPPED;
                    finished = true;
                    return;                
                }
            }
            
            addr_beg_index = addr_cur_index;
        }
        remaining_memory_size -= chunk_size;
    }
    
    /* Flush the remaining values on the batch */
    afs.write((char*)batch_addresses, batch_index*sizeof(uintptr_t));
    vfs.write((char*)batch_values, batch_index*memscanner.value_type_size);
    if (!afs || !vfs) {
        error = EOUTPUT;
        finished = true;
        return;
    }
    new_memory_size += batch_index*memscanner.value_type_size;
    finished = true;
}
