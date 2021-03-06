/*
 * GenericObject.cpp
 *
 *  Created on: Mar 20, 2017
 *      Author: Swee Balachandran
 */

#include "GenericObject.h"

GenericObject::GenericObject(double _time,genericObjectType_e type_in,int id_in,char *callsign_in,float lat_in,float lon_in,float altmsl_in,
		                          float vx_in,float vy_in,float vz_in){
    time = _time;
	id = id_in;
	type = type_in;
	memcpy(callsign,callsign_in,25*sizeof(char));
	pos = Position::makeLatLonAlt(lat_in,"degree",lon_in,"degree",altmsl_in,"m");
	vel = Velocity::makeVxyz(vx_in,vy_in,"m/s",vz_in,"m/s");
	orientation = vel.trk();

	if (callsign[0] == '\0'){
        callsign_avail = false;
	}else{
		callsign_avail = true;
	}

}

bool GenericObject::isEqual(GenericObject& obj, bool update){

	bool same;
	if(!callsign_avail){
        same = id == obj.id;
	}else{
		same = strcmp(callsign,obj.callsign) == 0;
	}

	if(same){
		if(update){
			time = obj.time;
			pos = obj.pos;
			vel = obj.vel;
			orientation = obj.orientation;
		}
		return true;
	}else{
		return false;
	}
}

int GenericObject::AddObject(list<GenericObject>& objectList,GenericObject& obj){

    bool inList = false;
    std::list<GenericObject>::iterator it;
    for(it = objectList.begin();it != objectList.end();it++){
        if( it->isEqual(obj,true)){
            inList = true;
            break;
        }
    }

    if(!inList){
        objectList.push_back(obj);
        return 1;
    }

	return 0;
}

void GenericObject::RemoveObject(list<GenericObject>& objectList, GenericObject& obj){
		objectList.remove(obj);
}

