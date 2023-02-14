/******************************************************************************
*                 SOFA, Simulation Open-Framework Architecture                *
*                    (c) 2006 INRIA, USTL, UJF, CNRS, MGH                     *
*                                                                             *
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU General Public License as published by the Free  *
* Software Foundation; either version 2 of the License, or (at your option)   *
* any later version.                                                          *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for    *
* more details.                                                               *
*                                                                             *
* You should have received a copy of the GNU General Public License along     *
* with this program. If not, see <http://www.gnu.org/licenses/>.              *
*******************************************************************************
* Authors: The SOFA Team and external contributors (see Authors.txt)          *
*                                                                             *
* Contact information: contact@sofa-framework.org                             *
******************************************************************************/
#ifndef OPTITRACKNATNETCLIENT_H
#define OPTITRACKNATNETCLIENT_H

#include <OptiTrackNatNet/config.h>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <iostream>

#include <sofa/core/objectmodel/BaseObject.h>
//#include <sofa/core/behavior/BaseController.h>
#include <sofa/type/Vec.h>
#include <sofa/type/Quat.h>
#include <sofa/component/controller/Controller.h>

namespace SofaOptiTrackNatNet
{

/// internal message buffer class, as defined in NatNet SDK
struct sPacket;

/// decoded definition of tracked objects
struct ModelDef;

/// decoded frame of tracked data
struct FrameData;

class OptiTrackNatNetDataReceiver : public sofa::component::controller::Controller
{
public:
    SOFA_ABSTRACT_CLASS(OptiTrackNatNetDataReceiver, sofa::component::controller::Controller);
protected:
    virtual ~OptiTrackNatNetDataReceiver() {}
public:
    virtual void processModelDef(const ModelDef* data) = 0;
    virtual void processFrame(const FrameData* data) = 0;
};

class OptiTrackNatNetClient :  public virtual sofa::core::objectmodel::BaseObject
{
public:
    SOFA_CLASS(OptiTrackNatNetClient,sofa::core::objectmodel::BaseObject);

protected:
    bool connect();
    void handleEvent(sofa::core::objectmodel::Event *) override;

    virtual void update();

public:
    sofa::core::objectmodel::Data<std::string> serverName; ///< NatNet server address (default to localhost)
    sofa::core::objectmodel::Data<std::string> clientName; ///< IP to bind this client to (default to localhost)
    sofa::core::objectmodel::Data<double> scale; ///< Scale factor to apply to coordinates (using the global frame as fixed point)

    sofa::core::objectmodel::Data<sofa::type::vector<sofa::type::Vec3f> > trackedMarkers; ///< Position of received known markers
    sofa::core::objectmodel::Data<sofa::type::vector<sofa::type::Vec3f> > otherMarkers; ///< Position of received unknown markers

    sofa::core::objectmodel::MultiLink<OptiTrackNatNetClient, OptiTrackNatNetDataReceiver, 0> natNetReceivers;

    OptiTrackNatNetClient();
    virtual ~OptiTrackNatNetClient();

    virtual void init() override;
    virtual void reinit() override;

    virtual void draw(const sofa::core::visual::VisualParams* vparams) override;

    sofa::core::objectmodel::Data<float> drawTrackedMarkersSize; ///< Size of displayed markers
    sofa::core::objectmodel::Data<sofa::type::RGBAColor> drawTrackedMarkersColor; ///< Color of displayed markers
    sofa::core::objectmodel::Data<float> drawOtherMarkersSize; ///< Size of displayed unknown markers
    sofa::core::objectmodel::Data<sofa::type::RGBAColor> drawOtherMarkersColor; ///< Color of displayed unknown markers

public:

protected:
    boost::asio::ip::udp::endpoint server_endpoint;
    boost::asio::ip::udp::socket* command_socket;
    boost::asio::ip::udp::socket* data_socket;

    sPacket* recv_command_packet;
    boost::asio::ip::udp::endpoint recv_command_endpoint;

    sPacket* recv_data_packet;
    boost::asio::ip::udp::endpoint recv_data_endpoint;
    void start_command_receive();
    void handle_command_receive(const boost::system::error_code& error,
            std::size_t bytes_transferred);

    void start_data_receive();
    void handle_data_receive(const boost::system::error_code& error,
            std::size_t bytes_transferred);
    void decodeFrame(const sPacket& data);
    void decodeModelDef(const sPacket& data);

    virtual void processFrame(const FrameData* data);
    virtual void processModelDef(const ModelDef* data);

    std::string serverString;
    sofa::type::fixed_array<unsigned char,4> serverVersion; // sending app's version [major.minor.build.revision]
    sofa::type::fixed_array<unsigned char,4> natNetVersion; // sending app's NatNet version [major.minor.build.revision]

    bool serverInfoReceived;
    bool modelInfoReceived;

    static boost::asio::io_service& get_io_service();
    static boost::asio::ip::udp::resolver& get_resolver();

};


struct PointCloudDef
{
    const char* name;
    int nMarkers;
    struct Marker
    {
        const char* name;
    };
    Marker* markers;
};

struct RigidDef
{
    const char* name;
    int ID;
    int parentID;
    sofa::type::Vec3f offset;
};

struct SkeletonDef
{
    const char* name;
    int ID;
    int nRigids;
    RigidDef* rigids;
};

struct ModelDef
{
    int nPointClouds;
    PointCloudDef* pointClouds;
    int nRigids;
    RigidDef* rigids;
    int nSkeletons;
    SkeletonDef* skeletons;
};

struct PointCloudData
{
    const char* name;
    int nMarkers;
    const sofa::type::Vec3f* markersPos;
};

struct RigidData
{
    int ID;
    sofa::type::Vec3f pos;
    sofa::type::Quat<SReal> rot;
    int nMarkers;
    const sofa::type::Vec3f* markersPos;
    const int* markersID; // optional (2.0+)
    const float* markersSize; // optional (2.0+)
    float meanError; // optional (2.0+)
};

struct SkeletonData
{
    int ID;
    int nRigids;
    RigidData* rigids;
};

struct FrameData
{
    int frameNumber;
    int nPointClouds;
    PointCloudData* pointClouds;
    int nRigids;
    RigidData* rigids;
    int nSkeletons;
    SkeletonData* skeletons;

    float latency;
    // unidentified markers
    int nOtherMarkers;
    const sofa::type::Vec3f* otherMarkersPos;
};

} // namespace SofaOptiTrackNatNet

#endif /* OPTITRACKNATNETCLIENT_H */
