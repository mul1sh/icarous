//
// Created by Swee Balachandran on 12/14/17.
//
#include <cstring>
#include "PathPlanner.h"

using namespace std;

PathPlanner::PathPlanner(FlightData* fd) {
    fdata = fd;
}

int64_t PathPlanner::FindPath(algorithm search, char *planID, double *fromPosition, double *toPosition,
                              double velocity[]) {

    int64_t retval = -1;

    std::list<Plan>::iterator it;
    for(it=flightPlans.begin();it != flightPlans.end(); ++ it){
        if (strcmp(it->getName().c_str(),planID)){
            continue;
        }
        flightPlans.erase(it);
        break;
    }

    switch(search){

        case _ASTAR_:
            retval = FindPathAstar(planID,fromPosition,toPosition);
            break;
        case _RRT_:
            retval = FindPathRRT(planID,fromPosition,toPosition,velocity);
            break;
        default:
            break;
    }

    return retval;
}

Plan PathPlanner::ComputeGoAbovePlan(Position start,Position goal,double altFence,double rSpeed){
    // Compute go above plan
    Plan ResolutionPlan2;
    double ETA = 0;
    double distH,distV;

    NavPoint nvpt1(start,ETA);
    ResolutionPlan2.addNavPoint(nvpt1);

    // Second waypoint directly above WP1
    Position wp2 = start.mkAlt(altFence+1);
    distV = wp2.distanceV(start);
    ETA = ETA + distV/rSpeed;
    NavPoint nvpt2(wp2,ETA);
    ResolutionPlan2.addNavPoint(nvpt2);

    // Third waypoint directly above exit point
    Position wp3 = goal.mkAlt(altFence+1);
    distH = wp3.distanceH(wp2);
    ETA = ETA + distH/rSpeed;
    NavPoint nvpt3(wp3,ETA);
    ResolutionPlan2.addNavPoint(nvpt3);

    // Final waypoint
    distV = goal.distanceH(wp3);
    ETA = ETA + distV/rSpeed;
    NavPoint nvpt4(goal,ETA);
    ResolutionPlan2.addNavPoint(nvpt4);

    return ResolutionPlan2;
}

void PathPlanner::GetWaypoint(char *planID, int wpID, double *waypoint) {
    for(Plan pl: flightPlans){
        if(strcmp(pl.getName().c_str(),planID)){
            continue;
        }else{
            Position pos = pl.getPos(wpID);
            waypoint[0] = pos.latitude();
            waypoint[1] = pos.longitude();
            waypoint[2] = pos.alt();
            break;
        }
    }
}

Plan* PathPlanner::GetPlan(char planID[]){
    std::list<Plan>::iterator it;
    Plan *fp;
    for(it=flightPlans.begin();it != flightPlans.end(); ++ it){
        if (strcmp(it->getName().c_str(),planID)){
            continue;
        }
        fp = &(*it);
        return fp;
    };

    return NULL;
}

void PathPlanner::OutputFlightPlan(ENUProjection* proj,char* planID,char* fenceFile,char* waypointFile){
    for(Plan pl: flightPlans){
        if(strcmp(pl.getName().c_str(),planID)){
            continue;
        }else {
            std::ofstream fp1;
            std::ofstream fp2;

            if (pl.size() > 0)
                fp1.open(waypointFile);
            else
                break;

            int fenceSize = fdata->GetTotalFences();
            if ( fenceSize > 0) {
                fp2.open(fenceFile);
                for(int i=0;i<fenceSize;i++){
                    fence *gf = fdata->GetGeofence(i);
                    for(int j=0;j<gf->GetSize();j++){
                        Vect3 loc = proj->project(gf->GetPolyMod()->getVertex(j));
                        fp2 << gf->GetID() <<" "<<loc.x<<" "<<
                                                  loc.y<<std::endl;
                    }

                }
                fp2.close();
            }

            for(int i=0;i<pl.size();i++){
                Position pos = pl.getPos(i);
                Vect3 X = proj->project(pos);
                fp1 << X.x <<" "<<X.y<<" "<<X.z<<std::endl;
            }

            fp1.close();

            break;
        }
    }
}

int PathPlanner::GetTotalWaypoints(char planID[]){
    Plan* fp = GetPlan(planID);
    if (fp != NULL)
        return fp->size();
    else
        return 0;
}

void PathPlanner::InputFlightPlan(char planID[],int wpID,double waypoint[],double speed){
    Position pos = Position::makeLatLonAlt(waypoint[0],"degree",waypoint[1],"degree",waypoint[2],"m");
    std::list<Plan>::iterator it;
    Plan* fp = GetPlan(planID);

    if(fp != NULL) {
        fp->add(pos, (double) wpID);
        return;
    }
    else{
        string planName = planID;
        Plan newPlan(planName);
        newPlan.add(pos,(double)wpID);
        flightPlans.push_back(newPlan);
        return;
    }
}

double PathPlanner::Dist2Waypoint(double currPosition[],double nextPosition[]){
    Position A = Position::makeLatLonAlt(currPosition[0],"degree",currPosition[1],"degree",currPosition[2],"m");
    Position B = Position::makeLatLonAlt(nextPosition[0],"degree",nextPosition[1],"degree",nextPosition[2],"m");
    return A.distanceH(B);
}


double PathPlanner::ComputeXtrackDistance(Plan* fp,int leg,double position[],double offset[]){
    std::list<Plan>::iterator it;
    Position pos = Position::makeLatLonAlt(position[0],"degree",position[1],"degree",position[2],"m");
    Position PrevWP     = fp->point(leg - 1).position();
    Position NextWP     = fp->point(leg).position();
    double psi1         = PrevWP.track(NextWP) * 180/M_PI;
    double psi2         = PrevWP.track(pos) * 180/M_PI;
    double sgn          = 0;
    if( (psi1 - psi2) >= 0){
        sgn = 1;              // Vehicle left of the path
    }
    else if( (psi1 - psi2) <= 180){
        sgn = -1;             // Vehicle right of the path
    }
    else if( (psi1 - psi2) < 0 ){
        sgn = -1;             // Vehicle right of path
    }
    else if ( (psi1 - psi2) >= -180  ){
        sgn = 1;              // Vehicle left of path
    }
    double bearing = std::abs(psi1 - psi2);
    double dist = PrevWP.distanceH(pos);
    double crossTrackDeviation = sgn*dist*sin(bearing * M_PI/180);
    double crossTrackOffset    = dist*cos(bearing * M_PI/180);

    if(offset != NULL){
        offset[0] = crossTrackDeviation;
        offset[1] = crossTrackOffset;
    }

    return crossTrackDeviation;
}


void PathPlanner::GetPositionOnPlan(Plan* fp,int leg,double currentPos[],double position[]){

    double offsets[2];
    ComputeXtrackDistance(fp,leg,currentPos,offsets);

    Position nextWP       = fp->point(leg).position();
    Position prevWP       = fp->point(leg-1).position();

    double headingNextWP  = prevWP.track(nextWP);;
    double dn             = offsets[1]*cos(headingNextWP);
    double de             = offsets[1]*sin(headingNextWP);
    Position cp           = prevWP.linearEst(dn, de);

    if(cp.alt() <= 0){
        cp = cp.mkAlt(nextWP.alt());
    }

    position[0] = cp.latitude();
    position[1] = cp.longitude();
    position[2] = cp.altitude();
}


double PathPlanner::GetInterceptHeadingToPlan(Plan* fp,int leg,double currentPos[]){
    Position pos = Position::makeLatLonAlt(currentPos[0],"degree",currentPos[1],"degree",currentPos[2],"m");

    double _positiontOnPlan[3];
    GetPositionOnPlan(fp,leg,currentPos,_positiontOnPlan);
    Position goal = Position::makeLatLonAlt(_positiontOnPlan[0],"degree",_positiontOnPlan[1],"degree",_positiontOnPlan[2],"m");
    return pos.track(goal)*180/M_PI;
}

double PathPlanner::ComputeXtrackDistance(char *planID, int leg, double *position, double *offset) {
    Plan* fp = GetPlan(planID);
    return ComputeXtrackDistance(fp,leg,position,offset);
}

void PathPlanner::GetPositionOnPlan(char *planID, int leg, double *currentPos, double *position) {
    Plan* fp = GetPlan(planID);
    GetPositionOnPlan(fp,leg,currentPos,position);
}

double PathPlanner::GetInterceptHeadingToPlan(char *planID, int leg, double *currentPos) {
    Plan* fp = GetPlan(planID);
    GetInterceptHeadingToPlan(fp,leg,currentPos);
}

void PathPlanner::ManueverToIntercept(Plan* fp,int leg,double currPosition[],double velocity[]){
    double xtrkDevGain ;
    double resolutionSpeed;
    double allowedDev;
    double Vs,Vf,V,sgn;
    double Trk;
    Position prevWP,nextWP,currentPos,cp;
    Plan currentFP;

    xtrkDevGain     = fdata->paramData.getValue("XTRK_GAIN");
    resolutionSpeed = fdata->paramData.getValue("RES_SPEED");
    allowedDev      = fdata->paramData.getValue("XTRK_DEV");

    double offsets[2];
    ComputeXtrackDistance(fp,leg,currPosition,offsets);
    double crossTrackDeviation = offsets[0];

    prevWP = fp->point(leg - 1).position();
    nextWP = fp->point(leg).position();

    if(xtrkDevGain < 0){
        xtrkDevGain = -xtrkDevGain;
    }


    Vs = xtrkDevGain*crossTrackDeviation;
    V  = resolutionSpeed;

    if(Vs >= 0){
        sgn = 1;
    }
    else{
        sgn = -1;
    }

    if(pow(std::abs(Vs),2) >= pow(V,2)){
        Vs = V*sgn;
    }

    Vf = sqrt(pow(V,2) - pow(Vs,2));

    Trk = prevWP.track(nextWP);
    double Vn = Vf*cos(Trk) - Vs*sin(Trk);
    double Ve = Vf*sin(Trk) + Vs*cos(Trk);
    double Vu = 0;

    double track = atan2(Ve,Vn)*180/M_PI;
    if(track < 0){
        track = 360 + track;
    }

    velocity[0] = track;
    velocity[1] = resolutionSpeed;
    velocity[2] = 0;
}

void PathPlanner::ManueverToIntercept(char* planID,int leg,double currPosition[],double velocity[]){
    Plan* fp = GetPlan(planID);
    ManueverToIntercept(fp,leg,currPosition,velocity);
}