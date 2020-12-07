#pragma once

#include <cmath>
#include <stdio.h>
#include <iostream>
#include <vector>
#include "xn_gpio.hpp"
#include <pthread.h>
#include "xn_vec.hpp"

#ifdef PIO_VIRTUAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#endif

/*
arm positions(mm):

shoulder: 
    0: (0, 50, 0)
    1: (40, 65,  0) 

elbow0 (40, 230, 0)
elbow1 (40, 300, 0)

wrist (40, 475, 0)
*/

#define ORTH_TOLERANCE 1E-3


bool run=1;

namespace xn 
{ 
    struct Transform {
        struct Transform *parent;
        vec3 position;
        vec3 eulers;

        Transform() {
            parent = NULL;
            position = { 0,0,0};
            eulers = {0,0,0};
        }

        Transform(vec3 _position, vec3 _eulers = {0,0,0}) {
            parent = NULL;
            position = _position;
            eulers = _eulers;
        }
    };

    float clamp(float n, float minval, float maxval) {
        return std::max( minval, std::min( n, maxval));
    }

    namespace ik 
    {
        class IkChain 
        {
        public:
            int bone_count;
            int iterations;
            float delta = 0.001;

            float *bone_lengths;
            float chain_length = 0;
            Transform *bones;
            vec3 *positions;
            vec3 pole;
            vec3 *poles;

            IkChain() {}

            IkChain(int _bone_count, Transform ordered_bones[], vec3 &_pole, int _iterations = 10)  {
                bone_count = _bone_count;
                iterations = _iterations;
                pole = _pole;

                bones = new Transform[bone_count +1];
                bone_lengths = new float[bone_count +1];
                positions = new vec3[bone_count +1];
                poles = new vec3[bone_count-1];

                for ( int i = 0; i < bone_count; i++ ) {
                    bones[i] = ordered_bones[i];
                }
                for ( int i = 1; i < bone_count; i++ ) {
                    bones[i].parent = &bones[i-1]; 
                }

                Transform current = bones[bone_count-2];
                for ( int i = 0; i < bone_count; i++ ) {
                    bone_lengths[i] = 0;
                }

                for ( int i = bone_count-2; i>=0; i-- ) {
                    // bone_start_rotations[i] = current.rotation;
                    bone_lengths[i] = vec3::dist(bones[i+1].position, bones[i].position);
                    // printf("bone_lengths[%d] = %f\n", i, bone_lengths[i]);
                    chain_length += bone_lengths[i];
                }
            }

            void resolve(vec3& target) {
                // save a copy of each bone position
                for ( int i = 0; i < bone_count; i++ ) {
                    positions[i] = bones[i].position;
                }

                // printf("d = %f\n", vec3::dist_sqr(target, bones[0].position ));
                // printf("cl = %f\n", chain_length);
                if ( vec3::dist_sqr(target, bones[0].position ) > chain_length*chain_length ) {
                    vec3 dir = target - bones[0].position;
                    dir.normalize();
                    
                    for ( int i =1; i < bone_count; i++ ) {
                        positions[i] = positions[i-1] + dir*bone_lengths[i-1];
                        bones[i].position = positions[i];
                    }
                } else {
                    for ( int iter = 0; iter < iterations; iter++ ) { 
                        // top->bottom traversal, move joints towards target
                        for ( int i = bone_count-1; i > 0; i-- ) {
                            if ( i == bone_count-1 ) {
                                positions[i] = target;
                            } else {
                                vec3 directionChildToCurrentBone = positions[i] - positions[i+1];
                                directionChildToCurrentBone.normalize();
                                vec3 d = directionChildToCurrentBone * bone_lengths[i];
                                positions[i] = positions[i+1] + d;
                            }
                        }

                        // top->bottom traversal, move joints towards parent bones
                        for ( int i = 1; i < bone_count; i++ ) {
                            vec3 directionParentToCurrentBone = positions[i] - positions[i-1];
                            directionParentToCurrentBone.normalize();
                            vec3 d = directionParentToCurrentBone * bone_lengths[i-1];
                            positions[i] = positions[i-1] + d;
                        }

                        // early success
                        if ( vec3::dist_sqr(positions[bone_count-1], target) < delta*delta )
                            break;    
                    }

                    // resolve poles
                    for ( int i = 0; i < bone_count-2; i++) {
                        vec3& par = positions[i];
                        vec3& mid = positions[i+1];
                        vec3& child = positions[i+2];

                        vec3 norm = par - child;
                        norm.normalize();
                        if ( i > 0 )
                            poles[i] = {0,3,0};
                        else
                            poles[i] = pole;
                        // poles[i].y = mid.y+2;
                        // if ( i != 0 )
                        // poles[i] = {3, 0, 0};
                        vec3 proj_mid = mid - norm * vec3::dot(mid, norm);
                        vec3 proj_pole = poles[i] - norm* vec3::dot(poles[i], norm);
                        
                        proj_mid = proj_mid - par;
                        proj_pole = proj_pole - par;

                        // proj_mid.normalize();
                        // proj_pole.normalize();
                        
                        vec3 y_ax = vec3::cross(norm, proj_mid);
                        float x = vec3::dot(proj_pole, proj_mid);
                        float y = vec3::dot(proj_pole, y_ax);

                        // if ( x < SMEPSILON || y < SMEPSILON )
                        //     continue;
                        float ang = atan2f(y, x);


                        // for ( int j = 0; j < bone_count; j++ ) {
                            vec3 relpos = mid - par;
                            positions[i+1] = par + vec3::rotate_axis(relpos, norm, ang);
                        // }
                    }
                    
                    for ( int i = 0; i < bone_count; i++) {
                        bones[i].position = positions[i];
                    }
                }

            }

            void print() {
                for ( int i = 0; i < bone_count; i++ ) {
                    printf("bone[%d] = ", i);
                    bones[i].position.print();
                    printf("\tbone length: %f\n", bone_lengths[i] );
                    if ( bones[i].parent != NULL) {
                        Transform p = *bones[i].parent;
                        printf("\tparent = ");
                        p.position.print();
                    } else {
                        printf("\troot of chain\n");
                    }
                }
            }
        };
        
        void *move_servo_thread(void *argv) {
            pio::SmoothServo *s = (pio::SmoothServo*) argv;
            double step = 1.0/120;

            while ( run ) {
                // asymptotic lerp
                // s->servo.setAngle(s->servo.getAngle()*0.95 + s->target_ang*0.05);

                // bezier curve
                s->update(step);
                time_sleep(step);
            }
            return NULL;
        }

        class ServoChain {
        public:
            IkChain ideal_chain;
            std::vector<pio::SmoothServo> *servos;

            vec3 *positions;
            float arm_len;
            
            ServoChain(IkChain bonechain, std::vector<pio::ServoAngular> _servos[], std::vector<vec3> axes[], bool* loopvar, float _arm_len = 1 ) {      
                ideal_chain = bonechain;
                arm_len = _arm_len;

                servos = new std::vector<pio::SmoothServo>[bonechain.bone_count];
                for ( int i = 0; i < bonechain.bone_count; i++ ) {
                    int j=0;
                    for (pio::ServoAngular& s : _servos[i]) 
                        servos[i].push_back(pio::SmoothServo(s, axes[i][j], bonechain.positions[i]));

                    for (pio::SmoothServo& s: servos[i]) {
                        s.axis = axes[i][j++];
                        pthread_create(&s.tid, NULL, &move_servo_thread, &s);
                    }
                }
                
                positions = new vec3[ideal_chain.bone_count];
                for ( int i = 0; i < ideal_chain.bone_count; i++ ) 
                    positions[i] = bonechain.bones[i].position;
            }

            ~ServoChain() {
                for ( int i = 0; i < ideal_chain.bone_count; i++ ) {
                    for (pio::SmoothServo s : servos[i]) {
                        pthread_join(s.tid, NULL);
                    }
                }
            }

            void resolve(vec3& target) {
                vec3& pax = servos[0][0].axis;
                vec3 target_normalized = target / arm_len;
                vec3 pole = target_normalized - (pax * vec3::dot(target_normalized, pax) );
                if (pole.mag() < SMEPSILON )
                    pole = {3,0,0};
                else {
                    pole.normalize();
                    pole = pole*-5;
                }

                ideal_chain.pole = pole;
                ideal_chain.resolve(target_normalized);
                // printf("ideal chain:\n");
                // ideal_chain.print();
                for ( int i = 0; i < ideal_chain.bone_count-1; i ++ ) {
                    for ( int j = 0; j < servos[i].size(); j++ ) {
                        vec3 next_target = ideal_chain.bones[i+1].position - positions[i];
                        vec3 pos = positions[i+1] - positions[i];
                        vec3& ax = servos[i][j].axis;
                        vec3 proj_pos = pos - (ax * vec3::dot(pos, ax));
                        vec3 proj_target = next_target - (ax * vec3::dot(next_target, ax));

                        if ( proj_pos.mag() < SMEPSILON ) continue;

                        proj_pos.normalize();

                        vec3 y_ax = vec3::cross(ax, proj_pos);
                        float x = vec3::dot(proj_target, proj_pos);
                        float y = vec3::dot(proj_target, y_ax);
                        float ang = atan2f(y, x);
                        float s_ang;
                        if ( servos[i].size()>1 && j == 0)
                            s_ang = fmod(servos[i][j].target_angle + ang, servos[i][j].servo.max_angle);
                        else 
                            s_ang = clamp(servos[i][j].target_angle + ang, servos[i][j].servo.min_angle, servos[i][j].servo.max_angle);
                        
                        if ( s_ang < 0 ) {
                            s_ang = servos[i][j].servo.max_angle + s_ang;
                        }
                        ang = s_ang - servos[i][j].target_angle;

                        servos[i][j].target_angle = s_ang;
                        // servos[i][j].servo.setAngle(s_ang);

                        // rotate servos in child joints
                        for ( int k = i+1;  k < ideal_chain.bone_count; k++ ) {
                            vec3 relpos = positions[k] - positions[i];
                            positions[k] = positions[i] + vec3::rotate_axis(relpos, ax, ang);
                            for ( int l = 0; l < servos[k].size(); l++ ) {
                                servos[k][l].axis = vec3::rotate_axis(servos[k][l].axis, ax, ang);
                                servos[k][l].axis.normalize();
                            }
                        }

                        // rotate servos axes in current joint
                        for ( int k = j+1; k < servos[i].size(); k++ ) {
                            servos[i][k].axis = vec3::rotate_axis(servos[i][k].axis, ax, ang);
                            servos[i][k].axis.normalize();
                        }
                    }
                }
            }

            void reset() {
                for ( int i = 0; i < ideal_chain.bone_count; i++ ) {
                    for ( pio::SmoothServo& s : servos[i] ) {
                        s.servo.setPosition(0.5);
                    }
                }
            }
        };
    }
}