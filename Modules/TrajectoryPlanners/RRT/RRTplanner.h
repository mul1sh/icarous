//
// Created by Swee Balachandran on 12/7/17.
//

#ifndef RRTPLANNER_H
#define RRTPLANNER_H

#include "Vect3.h"
#include "Poly3D.h"
#include "Position.h"
#include "Velocity.h"
#include "CDPolycarp.h"
#include "Projection.h"
#include "EuclideanProjection.h"
#include "Plan.h"
#include "Daidalus.h"
#include <string.h>
#include <list>

using namespace larcfm;

struct node{
    int id;
    Vect3 pos;
    Vect3 vel;
    std::vector<Vect3> trafficPos;
    std::vector<Vect3> trafficVel;

    bool goal;
    double g,h;
    double waitTime;
    struct node* parent;
    std::list<struct node> children;
};
typedef struct node node_t;


class RRTplanner {

private:

    /* min,max values for the 3D coordinates to restrict search */
    double xmax,xmin;
    double ymax,ymin;
    double zmax,zmin;

    /* Time step used for the 2nd Order Runge-Kutta method */
    double dT;

    /* Number of Runge-Kutta iterations to perform in one RRT exploration step */
    int dTsteps;

    /* boundingBox defines the extent of the search space */
    Poly3D boundingBox;

    /* List of obstacles */
    std::list<Poly3D> obstacleList;

    /* List of all nodes generated by the algorithm */
    std::list<node_t> nodeList;

    /* Iterator for the above node list*/
    std::list<node_t>::iterator ndit;

    /**
     * Dynamic model of the vehicle (see main.cpp for example)
     * @param X: Dynamics of the ownship and traffic [ownship,traffic]
     *           ownship = [x,xdot,y,ydot,z,zdot]
     *           traffic = [traffic_0,....,traffic_n], n is the last argument to this function.
     *           traffic_i = [tx,txdot,ty,tydot,tz,tzdot]
     * @param U: Array of control inputs U[0],U[1],U[2] for the ownship
     * @param Y: Output
     * @param n: number of traffic vehicles.
     */
    void (*F)(double [],double [],double [],int);


    /**
     * Control input function
     * @param nn: Nearest node in the nodeList
     * @param rn: Random node generated.
     * @param U: computed control U[0],U[1],U[2]
     */
    void (*InputFunction)(node_t&,node_t&,double []);

    /* POLYCarp class for detecting obstacle violations */
    CDPolycarp geoPolycarp;

    /* Total node explored*/
    int nodeCount;

    /* Root node */
    node_t root;

    /* Total traffic vehicles*/
    int trafficSize;

    /* Closet distance to goal*/
    double closestDist;

    /* DAA lookahead parameter*/
    double daaLookAhead;

    /* Avoidance radius*/
    double maxD;

    /* Closest node to goal */
    node_t* closestNode;

    /* Goal node */
    node_t goalNode;

    /* DAA object */
    Daidalus DAA;

    double maxInputNorm;
    bool goalreached;
    time_t startTime;


public:

    /**
     * Constructor
     * @param boundary search space boundary
     * @param stepT number of iterations of RK method in one RRT exploration
     * @param dt step size for RK method
     * @param maxD radius to stay clear of for collision
     * @param PlantDynamics ODE for ownship and traffic
     * @param ControlInput control input function
     * @param daaConfig daa input file name
     */
    RRTplanner(Poly3D &boundary,int stepT,double dt,double maxD,double maxInputNorm,
               void (*PlantDynamics)(double [],double [],double [],int),
               void (*ControlInput)(node_t&, node_t&,double[]),
               const char daaConfig[]);


    void SetParameters(int stepT,double dt,double maxD,double maxInputNorm);
    void SetDAAParameters(std::string parameterList);

    /**
     * Initialize the problem
     * @param Pos initial position of ownship
     * @param Vel initial velocity of ownship
     * @param obstacleList list of obstacles
     * @param TrafficPos list of traffic positions
     * @param TrafficVel list of traffic velocities
     * @param goal goal location
     */
    void Initialize(Vect3& Pos,Vect3& Vel,std::list<Poly3D> &obstacleList,
                    std::vector<Vect3>& TrafficPos,std::vector<Vect3>& TrafficVel,node_t& goal);

    void MotionModel(node_t& nearest,node_t& outputNode, double U[]);

    bool CheckFenceCollision(Vect3& qPos);
    bool CheckTrafficCollision(Vect3& qPos,Vect3& qVel,std::vector<Vect3>& TrafficPos,std::vector<Vect3>& trafficVel);
    bool CheckTrafficCollisionWithBands(bool CheckTurn,Vect3& qPos,Vect3& qVel,
                                        std::vector<Vect3>& TrafficPos,std::vector<Vect3>& trafficVel,Vect3& oldVel);
    bool CheckTurnConflict(double low,double high,double newHeading,double oldHeading);
    void GetInput(node_t& nn, node_t& qn,double U[]);
    node_t* FindNearest(node_t& query);
    double NodeDistance(node_t& A,node_t& B);
    void RRTStep();
    bool CheckGoal();
    Plan GetPlan(EuclideanProjection& proj);
    void SetGoal(node_t& goal);
    bool CheckDirectPath2Goal(node_t* nearest);
    bool LinePlanIntersection(Vect2& A,Vect2& B,double floor,double ceiling,Vect3& currentPos,Vect3& nextPos);
    bool CheckProjectedFenceConflict(node_t* qnode,node_t* goal);
    bool CheckWaitAndGo(node_t& qnode);
    std::list<node_t>* GetNodeList();

};


#endif //RRT_RRTPLANNER_H
