#include "dtrack.hpp"
#include "spdlog/spdlog.h"

DTrack::DTrack(const std::string& connection)
  : m_dtrack_sdk(connection) {
  if (connection.empty()) {
    spdlog::warn("No dtrack connection specified. Use --dtrack=ip:port to establish a dtrack connection");
    return;
  }

  const auto log_expect_true = [](std::string_view name, bool value) {
    const auto level = value ? spdlog::level::info : spdlog::level::warn;
    spdlog::log(level, "[DTrack] {}: {}", name, value);
  };

  spdlog::info("[DTrack] Connecting to dtrack: {}", connection);
  log_expect_true("Command Interface Valid", m_dtrack_sdk.isCommandInterfaceValid());
  log_expect_true("Data Interface Valid", m_dtrack_sdk.isDataInterfaceValid());
  log_expect_true("Local Data Port Valid", m_dtrack_sdk.isLocalDataPortValid());
  log_expect_true("Data Port", m_dtrack_sdk.getDataPort());
  log_expect_true("TCP Valid", m_dtrack_sdk.isTCPValid());
  log_expect_true("UDP Valid", m_dtrack_sdk.isUDPValid());

  if (std::string access; m_dtrack_sdk.getParam("system", "access", access)) {
    spdlog::info("[DTrack] Access: {}", access);
  } else {
    LogError();
  }

  if (std::string status; m_dtrack_sdk.getParam("status", "active", status)) {
    spdlog::info("[DTrack] Status: {}", status);

    if (status != "mea") {
      spdlog::info("[DTrack] Start measurement");
      if (!m_dtrack_sdk.startMeasurement()) {
        LogError();
      }
    }

  } else {
    LogError();
  }
}

DTrack::~DTrack() {
  m_quit = true;
  if (m_receive_thread.joinable()) {
    m_receive_thread.join();
  }

  if (m_dtrack_sdk.isCommandInterfaceValid()) {
    if (!m_dtrack_sdk.stopMeasurement()) {
      LogError();
    }
  }
}

void DTrack::ReceiveThread() {
  while (!m_quit.load(std::memory_order_relaxed)) {
    if (m_dtrack_sdk.receive()) {
      auto new_data = GenerateJSON();
      std::unique_lock<std::mutex> lock(m_data_mutex);
      std::swap(m_tracking_data, new_data);
    } else {
      LogError();
    }
  }
}

nlohmann::json DTrack::GenerateJSON() {
  const auto serialize_body = [](const DTrackBody* body) {
    assert(body);

    nlohmann::json serialized_body = nlohmann::json::object();
    serialized_body["id"] = body->id;
    serialized_body["isTracked"] = body->isTracked();

    if (body->isTracked()) {
      serialized_body["position"] = body->loc;
      serialized_body["orientation"] = body->rot;
    }

    return serialized_body;
  };

  nlohmann::json bodies = nlohmann::json::array();
	for (int i = 0; i < m_dtrack_sdk.getNumBody(); ++i) {
    bodies[i] = serialize_body(m_dtrack_sdk.getBody(i));
  }

  nlohmann::json data = {
    {"frame", m_dtrack_sdk.getFrameCounter()},
    {"time", m_dtrack_sdk.getTimeStamp()},
    {"bodies", std::move(bodies)},
  };

  return data;
}

void DTrack::LogError() {
  switch (m_dtrack_sdk.getLastDataError()) {
  case DTrackSDK::Errors::ERR_NONE:
    break;

  case DTrackSDK::Errors::ERR_TIMEOUT:
    spdlog::error("Timeout while waiting for tracking data");
    break;

  case DTrackSDK::Errors::ERR_NET:
    spdlog::error("Error while receiving tracking data");
    break;

  case DTrackSDK::Errors::ERR_PARSE:
    spdlog::error("Error while parsing tracking data");
    break;
  }

  switch (m_dtrack_sdk.getLastServerError()) {
  case DTrackSDK::ERR_NONE:
    break;

  case DTrackSDK::ERR_TIMEOUT:
    spdlog::error("Timeout while waiting for controller command");
    break;

  case DTrackSDK::ERR_NET:
    spdlog::error("Error while receiving controller command");
    break;

  case DTrackSDK::ERR_PARSE:
    spdlog::error("Error while parsing controller command");
    break;
  }
}

// nlohmann::json serialize_body(const DTrackBody* body) {
//   assert(body);

//   nlohmann::json serialized_body = nlohmann::json::object();
//   serialized_body["id"] = body->id;

//   if (body->isTracked()) {
//     serialized_body["loc"] = body->loc;
//     serialized_body["rot"] = body->rot;
//   }

//   return serialized_body;
// }

// nlohmann::json serialize_data(DTrackSDK* dtrack) {

//   nlohmann::json bodies = nlohmann::json::array();
// 	for (int i = 0; i < dtrack->getNumBody(); ++i) {
//     bodies[i] = serialize_body(dtrack->getBody(i));
//   }

//   nlohmann::json data = {
//     {"frame", dtrack->getFrameCounter()},
//     {"time", dtrack->getTimeStamp()},
//     {"bodies", std::move(bodies)},
//   };

//   return data;
// }

// int main(int argc, char** argv){
//   while (!quit) {
//   }


// /**
//  * \brief Prints current tracking data to console.
//  */
// static void output_to_console()
// {
// 	std::cout.precision( 3 );
// 	std::cout.setf( std::ios::fixed, std::ios::floatfield );

// 	std::cout << std::endl << "frame " << dt->getFrameCounter() << " ts " << dt->getTimeStamp()
// 	          << " nbod " << dt->getNumBody() << " nfly " << dt->getNumFlyStick()
// 	          << " nmea " << dt->getNumMeaTool() << " nmearef " << dt->getNumMeaRef() 
// 	          << " nhand " << dt->getNumHand() << " nmar " << dt->getNumMarker() 
// 	          << " nhuman " << dt->getNumHuman() << " ninertial " << dt->getNumInertial()
// 	          << std::endl;


// 	// A.R.T. Flysticks:
// 	for ( int i = 0; i < dt->getNumFlyStick(); i++ )
// 	{
// 		const DTrackFlyStick* flystick = dt->getFlyStick( i );
// 		if ( flystick == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid Flystick id " << i << std::endl;
// 			break;
// 		}

// 		if ( ! flystick->isTracked() )
// 		{
// 			std::cout << "fly " << flystick->id << " not tracked" << std::endl;
// 		}
// 		else
// 		{
// 			std::cout << "flystick " << flystick->id << " qu " << flystick->quality
// 			          << " loc " << flystick->loc[ 0 ] << " " << flystick->loc[ 1 ] << " " << flystick->loc[ 2 ]
// 			          << " rot " << flystick->rot[ 0 ] << " " << flystick->rot[ 1 ] << " " << flystick->rot[ 2 ]
// 			          << " "     << flystick->rot[ 3 ] << " " << flystick->rot[ 4 ] << " " << flystick->rot[ 5 ]
// 			          << " "     << flystick->rot[ 6 ] << " " << flystick->rot[ 7 ] << " " << flystick->rot[ 8 ]
// 			          << std::endl;
// 		}

// 		std::cout << "      btn";
// 		for ( int j = 0; j < flystick->num_button; j++ )
// 		{
// 			std::cout << " " << flystick->button[ j ];
// 		}
// 		std::cout << " joy";
// 		for ( int j = 0; j < flystick->num_joystick; j++ )
// 		{
// 			std::cout << " " << flystick->joystick[ j ];
// 		}
// 		std::cout << std::endl;
// 	}

// 	// Measurement tools:
// 	for ( int i = 0; i < dt->getNumMeaTool(); i++ )
// 	{
// 		const DTrackMeaTool* meatool = dt->getMeaTool( i );
// 		if ( meatool == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid Measurement tool id " << i << std::endl;
// 			break;
// 		}

// 		if ( ! meatool->isTracked() )
// 		{
// 			std::cout << "mea " << meatool->id << " not tracked" << std::endl;
// 		}
// 		else
// 		{
// 			std::cout << "mea " << meatool->id << " qu " << meatool->quality
// 			          << " loc " << meatool->loc[ 0 ] << " " << meatool->loc[ 1 ] << " " << meatool->loc[ 2 ]
// 			          << " rot " << meatool->rot[ 0 ] << " " << meatool->rot[ 1 ] << " " << meatool->rot[ 2 ]
// 			          << " "     << meatool->rot[ 3 ] << " " << meatool->rot[ 4 ] << " " << meatool->rot[ 5 ]
// 			          << " "     << meatool->rot[ 6 ] << " " << meatool->rot[ 7 ] << " " << meatool->rot[ 8 ]
// 			          << std::endl;
// 		}

// 		if ( meatool->tipradius > 0.0 )
// 		{
// 			std::cout << "      radius " << meatool->tipradius << std::endl;
// 		}

// 		if ( meatool->num_button > 0 )
// 		{
// 			std::cout << "      btn";
// 			for ( int j = 0; j < meatool->num_button; j++ )
// 			{
// 				std::cout << " " << meatool->button[ j ];
// 			}
// 			std::cout << std::endl;
// 		}
// 	}

// 	// Measurement references:
// 	for ( int i = 0; i < dt->getNumMeaRef(); i++ )
// 	{
// 		const DTrackMeaRef* mearef = dt->getMeaRef( i );
// 		if ( mearef == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid Measurement reference id " << i << std::endl;
// 			break;
// 		}

// 		if ( ! mearef->isTracked() )
// 		{
// 			std::cout << "mearef " << mearef->id << " not tracked" << std::endl;
// 		}
// 		else
// 		{
// 			std::cout << "mearef " << mearef->id << " qu " << mearef->quality
// 			          << " loc " << mearef->loc[ 0 ] << " " << mearef->loc[ 1 ] << " " << mearef->loc[ 2 ]
// 			          << " rot " << mearef->rot[ 0 ] << " " << mearef->rot[ 1 ] << " " << mearef->rot[ 2 ]
// 			          << " "     << mearef->rot[ 3 ] << " " << mearef->rot[ 4 ] << " " << mearef->rot[ 5 ]
// 			          << " "     << mearef->rot[ 6 ] << " " << mearef->rot[ 7 ] << " " << mearef->rot[ 8 ]
// 			          << std::endl;
// 		}
// 	}

// 	// Single markers:
// 	for ( int i = 0; i < dt->getNumMarker(); i++ )
// 	{
// 		const DTrackMarker* marker = dt->getMarker( i );
// 		if ( marker == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid marker index " << i << std::endl;
// 			break;
// 		}

// 		std::cout << "mar " << marker->id << " qu " << marker->quality
// 		          << " loc " << marker->loc[ 0 ] << " " << marker->loc[ 1 ] << " " << marker->loc[ 2 ]
// 		          << std::endl;
// 	}

// 	// A.R.T. FINGERTRACKING hands:
// 	for ( int i = 0; i < dt->getNumHand(); i++ )
// 	{
// 		const DTrackHand* hand = dt->getHand( i );
// 		if ( hand == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid FINGERTRACKING id " << i << std::endl;
// 			break;
// 		}

// 		if ( ! hand->isTracked() )
// 		{
// 			std::cout << "hand " << hand->id << " not tracked" << std::endl;
// 		}
// 		else
// 		{
// 			std::cout << "hand " << hand->id << " qu " << hand->quality
// 			          << " lr " << ( ( hand->lr == 0 ) ? "left" : "right") << " nf " << hand->nfinger
// 			          << " loc " << hand->loc[ 0 ] << " " << hand->loc[ 1 ] << " " << hand->loc[ 2 ]
// 			          << " rot " << hand->rot[ 0 ] << " " << hand->rot[ 1 ] << " " << hand->rot[ 2 ]
// 			          << " "     << hand->rot[ 3 ] << " " << hand->rot[ 4 ] << " " << hand->rot[ 5 ]
// 			          << " "     << hand->rot[ 6 ] << " " << hand->rot[ 7 ] << " " << hand->rot[ 8 ]
// 			          << std::endl;

// 			for ( int j = 0; j < hand->nfinger; j++ )
// 			{
// 				std::cout << "       fi " << j
// 				          << " loc " << hand->finger[ j ].loc[ 0 ] << " " << hand->finger[ j ].loc[ 1 ] << " " << hand->finger[ j ].loc[ 2 ]
// 				          << " rot " << hand->finger[ j ].rot[ 0 ] << " " << hand->finger[ j ].rot[ 1 ] << " " << hand->finger[ j ].rot[ 2 ]
// 				          << " "     << hand->finger[ j ].rot[ 3 ] << " " << hand->finger[ j ].rot[ 4 ] << " " << hand->finger[ j ].rot[ 5 ]
// 				          << " "     << hand->finger[ j ].rot[ 6 ] << " " << hand->finger[ j ].rot[ 7 ] << " " << hand->finger[ j ].rot[ 8 ]
// 				          << std::endl;
// 				std::cout << "       fi " << j
// 				          << " tip " << hand->finger[ j ].radiustip
// 				          << " pha " << hand->finger[ j ].lengthphalanx[ 0 ] << " " << hand->finger[ j ].lengthphalanx[ 1 ]
// 				          << " "     << hand->finger[ j ].lengthphalanx[ 2 ]
// 				          << " ang " << hand->finger[ j ].anglephalanx[ 0 ] << " " << hand->finger[ j ].anglephalanx[ 1 ]
// 				          << std::endl;
// 			}
// 		}
// 	}

// 	// A.R.T human models:
// 	if ( dt->getNumHuman() < 1 )
// 	{
// 		std::cout << "no human model data" << std::endl;
// 	}

// 	for ( int i = 0; i < dt->getNumHuman(); i++ )
// 	{
// 		const DTrackHuman* human = dt->getHuman( i );
// 		if ( human == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid human model id " << i << std::endl;
// 			break;
// 		}

// 		if ( ! human->isTracked() )
// 		{
// 			std::cout << "human " << human->id << " not tracked" << std::endl;
// 		}
// 		else
// 		{
// 			std::cout << "human " << human->id << " num joints " << human->num_joints << std::endl;
// 			for ( int j = 0; j < human->num_joints; j++ )
// 			{
// 				if ( ! human->joint[ j ].isTracked() )
// 				{
// 					std::cout << "joint " << human->joint[ j ].id << " not tracked" << std::endl;
// 				}
// 				else
// 				{
// 					std::cout << "joint " << human->joint[ j ].id << " qu " << human->joint[j].quality
// 					          << " loc " << human->joint[ j ].loc[ 0 ] << " " << human->joint[j].loc[ 1 ] << " " << human->joint[ j ].loc[ 2 ]
// 					          << " rot " << human->joint[ j ].rot[ 0 ] << " " << human->joint[j].rot[ 1 ] << " " << human->joint[ j ].rot[ 2 ]
// 					          << " "     << human->joint[ j ].rot[ 3 ] << " " << human->joint[j].rot[ 4 ] << " " << human->joint[ j ].rot[ 5 ]
// 					          << " "     << human->joint[ j ].rot[ 6 ] << " " << human->joint[j].rot[ 7 ] << " " << human->joint[ j ].rot[ 8 ]
// 					          << std::endl;
// 				}
// 			}
// 		}
// 		std::cout << std::endl;
// 	}

// 	// Hybrid bodies:
// 	if ( dt->getNumInertial() < 1 )
// 	{
// 		std::cout << "no inertial body data" << std::endl;
// 	}

// 	for ( int i = 0; i < dt->getNumInertial(); i++ )
// 	{
// 		const DTrackInertial* inertial = dt->getInertial( i );
// 		if ( inertial == NULL )
// 		{
// 			std::cout << "DTrackSDK fatal error: invalid hybrid body id " << i << std::endl;
// 			break;
// 		}

// 		std::cout << " inertial body " << inertial->id << " st " << inertial->st << " error " << inertial->error << std::endl;
// 		if ( inertial->isTracked() )
// 		{
// 			std::cout << " loc " << inertial->loc[ 0 ] << " " << inertial->loc[ 1 ] << " " << inertial->loc[ 2 ]
// 			          << " rot " << inertial->rot[ 0 ] << " " << inertial->rot[ 1 ] << " " << inertial->rot[ 2 ]
// 			          << " "     << inertial->rot[ 3 ] << " " << inertial->rot[ 4 ] << " " << inertial->rot[ 5 ]
// 			          << " "     << inertial->rot[ 6 ] << " " << inertial->rot[ 7 ] << " " << inertial->rot[ 8 ]
// 			          << std::endl;
// 		}
// 	}
// }




// /**
//  * \brief Prints ATC messages to console.
//  */
// static void messages_to_console()
// {
// 	while ( dt->getMessage() )
// 	{
// 		std::cout << "ATC message: \"" << dt->getMessageStatus() << "\" \"" << dt->getMessageMsg() << "\"" << std::endl;
// 	}
// }
