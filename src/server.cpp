/* SPDX-License-Identifier: MIT */

/*
Copyright (c) 2025 Pluraf Embedded AB <code@pluraf.com>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to
do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/


extern "C" {
    #include <modbus.h>
}

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <string>
#include <array>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "channel_modbus_api.grpc.pb.h"


using channel_modbus_api::ModbusChannel;
using channel_modbus_api::ReadRequest;
using channel_modbus_api::ReadBitsResponse;
using channel_modbus_api::ReadRegistersResponse;
using channel_modbus_api::WriteBitsRequest;
using channel_modbus_api::WriteRegistersRequest;
using channel_modbus_api::WriteResponse;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;


class ModbusChannelImpl final: public ModbusChannel::Service{
    modbus_t * mb_;
public:
    ModbusChannelImpl(){
        mb_ = modbus_new_tcp("127.0.0.1", MODBUS_TCP_DEFAULT_PORT);
        modbus_connect(mb_);
    }

    Status ReadCoils(
        ServerContext * context,
        ReadRequest const * request,
        ReadBitsResponse * response
    )override{
        std::uint8_t data[MODBUS_MAX_READ_BITS];
        int ret = modbus_read_bits(mb_, request->address(), request->quantity(), data);
        response->set_status(ret);
        if(ret == -1){
            response->set_error(errno);
        }else{
            for(int ix = 0; ix < ret; ++ix){
                response->add_bits(data[ix]);
            }
        }
        return Status::OK;
    }

    Status ReadDiscreteInputs(
        ServerContext * context,
        ReadRequest const * request,
        ReadBitsResponse * response
    )override{
        std::uint8_t data[MODBUS_MAX_READ_BITS];
        int ret = modbus_read_input_bits(mb_, request->address(), request->quantity(), data);
        response->set_status(ret);
        if(ret == -1){
            response->set_error(errno);
        }else{
            for(int ix = 0; ix < ret; ++ix){
                response->add_bits(data[ix]);
            }
        }
        return Status::OK;
    }

    Status ReadHoldingRegisters(
        ServerContext * context,
        ReadRequest const * request,
        ReadRegistersResponse * response
    )override{
        std::uint16_t data[MODBUS_MAX_READ_REGISTERS];
        int ret = modbus_read_registers(mb_, request->address(), request->quantity(), data);
        response->set_status(ret);
        if(ret == -1){
            response->set_error(errno);
        }else{
            for(int ix = 0; ix < ret; ++ix){
                response->add_registers(data[ix]);
            }
        }
        return Status::OK;
    }

    Status ReadInputRegisters(
        ServerContext * context,
        ReadRequest const * request,
        ReadRegistersResponse * response
    )override{
        std::uint16_t data[MODBUS_MAX_READ_REGISTERS];
        int ret = modbus_read_input_registers(mb_, request->address(), request->quantity(), data);
        response->set_status(ret);
        if(ret == -1){
            response->set_error(errno);
        }else{
            for(int ix = 0; ix < ret; ++ix){
                response->add_registers(data[ix]);
            }
        }
        return Status::OK;
    }

    Status WriteCoils(
        ServerContext * context,
        WriteBitsRequest const * request,
        WriteResponse * response
    )override{
        if(request->bits_size() > MODBUS_MAX_WRITE_BITS){
            response->set_status(-1);
            response->set_error(EMBMDATA);
            return Status::OK;
        }
        // Prepare data
        std::array<uint8_t, MODBUS_MAX_WRITE_BITS> coils;
        for(int ix = 0; ix < request->bits_size(); ++ix){
            coils[ix] = request->bits(ix);
        }
        // Call libmodbus
        int ret = modbus_write_bits(mb_, request->address(), request->quantity(), coils.data());
        response->set_status(ret);
        if(ret == -1){
            response->set_error(errno);
        }
        return Status::OK;
    }

    Status WriteHoldingRegisters(
        ServerContext * context,
        WriteRegistersRequest const * request,
        WriteResponse * response
    )override{
        if(request->registers_size() > MODBUS_MAX_WRITE_REGISTERS){
            response->set_status(-1);
            response->set_error(EMBMDATA);
            return Status::OK;
        }
        // Prepare data
        std::array<uint16_t, MODBUS_MAX_WRITE_REGISTERS> registers;
        for(int ix = 0; ix < request->registers_size(); ++ix){
            registers[ix] = request->registers(ix);
        }
        // Call libmodbus
        int ret = modbus_write_registers(
            mb_, request->address(), request->quantity(), registers.data()
        );
        response->set_status(ret);
        if(ret == -1){
            response->set_error(errno);
        }
        return Status::OK;
    }
};


ABSL_FLAG(uint16_t, port, 1502, "Server port for the service");


int main(int argc, char **argv)
{
    absl::ParseCommandLine(argc, argv);
    std::string server_address = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
    ModbusChannelImpl service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
    return 0;
}