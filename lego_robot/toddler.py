#!/usr/bin/env python

'''
Robotics: Science and Systems
Mihail Dunaev
16 Nov 2015

Program for a lego robot that goes from room to room, avoids collisions and
looks for boxes with certain pictures on them (resources).
'''

__TODDLER_VERSION__="1.0.0"

import time
import numpy
import cv2
import datetime

# debugging
import sys, tty, termios

DESTINATION_ROOM = 'D'
DIRECTION_RIGHT = 1
DIRECTION_LEFT = 2

IRF_POS = 0
IRT_POS = 1
SONAR_POS = 2
LF_POS = 3

# collision threshold for IRR
IR_HIT_THRESHOLD = 300
IR_HIT_THRESHOLD_LEFT = 280
IR_WALL_THRESHOLD = 170
IR_WALL_THRESHOLD_LEFT = 140
IR_VOID_THRESHOLD = 100

OUTSIDE_WALL_DELAY1 = 2.2
OUTSIDE_WALL_DELAY2 = 2.2
OUTSIDE_WALL_DELAY2_LEFT = 2.5

RIGHT_STRAIGHT = -100
LEFT_STRAIGHT = 85

TIME_90 = 1.4
TIME_SERVO_180 = 1.3

CAMERA_DELAY = 5

GREEN_THRESHOLD = 2500
BLUE_THRESHOLD = 2000
WHITE_THRESHOLD = 2000
RED_THRESHOLD = 2000
ORANGE_THRESHOLD = 2000
BLACK_THRESHOLD = 2000
YELLOW_THRESHOLD = 2000

MARIOK = 43
WARIOK = 40
FRYK = 11
ZOIDBERGK = 12

MARIO_RES = 0
WARIO_RES = 1
FRY_RES = 2
ZOIDBERG_RES = 3

EVENT_TURN = 1
EVENT_NR = 2
EVENT_END = 3

SERVO_ANGLE_FRONT = 98

LOG = True

class Toddler:

    def __init__(self, IO):

        # init stuff
        print 'Toddler started'
        self.IO = IO
        
        # motor init
        self.mr = self.ml = 0

        # servo init - assume lazy implementation of the servo
        self.serv_angle = 0
        self.IO.servoEngage()
        self.IO.servoSet(self.serv_angle)
        time.sleep(TIME_SERVO_180)

        # HSV ranges for colors
        self.lower_blue = numpy.array([103,150,50])
        self.upper_blue = numpy.array([107,255,255])
        self.lower_green = numpy.array([65,150,50])
        self.upper_green = numpy.array([76,255,255])
        self.lower_orange = numpy.array([8,150,50])
        self.upper_orange = numpy.array([10,255,255])
        self.lower_red = numpy.array([2,150,50])
        self.upper_red = numpy.array([5,255,255])
        self.lower_yellow = numpy.array([19,150,50])
        self.upper_yellow = numpy.array([24,255,255])
        self.lower_white = numpy.array([0,0,207])
        self.upper_white = numpy.array([255,12,216])
        self.lower_black = numpy.array([0,0,0])
        self.upper_black = numpy.array([255,255,255])
        
        # load cascades
        self.mario_cascade = cv2.CascadeClassifier("mario_cascade.xml")
        self.wario_cascade = cv2.CascadeClassifier("wario_cascade.xml")
        self.fry_cascade = cv2.CascadeClassifier("fry_cascade.xml")
        self.zoidberg_cascade = cv2.CascadeClassifier("zoidberg_cascade.xml")

        self.known_room = False
        self.room = 0
        self.dest_room = 0

        # res detection
        self.scan_room = True
        self.res_type = MARIO_RES
        self.res_str = "Mario"
        self.res_detected = False
        self.res = (-1,-1,-1,-1)

        # logging
        self.flog = None
        if LOG:
            self.flog = open('/home/student/log', 'w')

    def Control(self, OK):

        first_move = True
        direction = DIRECTION_RIGHT
        event = 0
        num_corners = 0
        backwards = False

        while True:
        
            # wait for a full 360 scan
            if self.scan_room == True:
                time.sleep(1)
                continue

            if self.res_detected == True:
                break

            if first_move == True:
                self.get_to_wall()
                self.align_direction(direction)
                first_move = False

            event = self.follow_wall(direction,True,True,False)
            if event == EVENT_TURN:
                if backwards == True:
                    if num_corners > 0:
                        num_corners = num_corners - 1
                        continue
                else:
                    num_corners = num_corners + 1
                self.scan_room = True
                continue

            elif event == EVENT_NR:
                if direction == DIRECTION_LEFT:
                    print "Couldn't find the resource in the room"
                    self.IO.setError("flash", cnt = 4)
                    break
                self.turn(180,'l')
                direction = DIRECTION_LEFT
                event = self.follow_wall(direction,False,True,False)
                backwards = True

        # rotate to servo angle
        tmp_da = 0
        if self.serv_angle < SERVO_ANGLE_FRONT:
            self.turn(SERVO_ANGLE_FRONT-self.serv_angle,'r')
            tmp_da = SERVO_ANGLE_FRONT - self.serv_angle
        else:
            self.turn(self.serv_angle-SERVO_ANGLE_FRONT,'l')
            tmp_da = self.serv_angle - SERVO_ANGLE_FRONT

        self.serv_angle = SERVO_ANGLE_FRONT
        self.IO.servoSet(self.serv_angle)
        self.sleep_servo(tmp_da)

        # move until you hit it
        if self.res_detected == True:
            self.trap_resource()

        while OK():
            time.sleep(0.05)

        #while True:
        #    if self.known_room == True:
        #        break
        #    time.sleep(1)

        # get to closest wall
        #self.get_to_wall()
        #direction = 0

        #self.dest_room = self.translate_room(DESTINATION_ROOM)
        #direction = self.find_direction()
        #direction = DIRECTION_LEFT

        # align_direction
        #self.align_direction(direction)

        # follow wall
        #self.follow_wall(direction)


    def Vision(self, OK):

        # camera init
        self.IO.cameraSetResolution("medium")

        while True:
        
            # wait after control
            if self.scan_room == False:
                time.sleep(1)
                continue

            # do a 360 scan
            self.res_detected = self.detect_resource()
        
            if self.res_detected:
                self.IO.setStatus("flash", cnt = 2)
                det = True
                eps = 10
                delta_angle = 20
                prev_direction = -1
                det_direction = -1
        
                # turn camera until obj is in center
                while True:
                    if not det:
                        if delta_angle < 10:
                            delta_angle = delta_angle - 1
                        else:
                            delta_angle = delta_angle/2
                        if prev_direction == DIRECTION_RIGHT:
                            self.serv_angle = self.serv_angle + delta_angle
                            self.IO.servoSet(self.serv_angle)
                            self.sleep_servo(delta_angle)
                            prev_direction = DIRECTION_LEFT
                        elif prev_direction == DIRECTION_LEFT:
                            self.serv_angle = self.serv_angle - delta_angle
                            self.IO.servoSet(self.serv_angle)
                            self.sleep_servo(delta_angle)
                            prev_direction = DIRECTION_RIGHT
                    else:
                        xmid = self.res[0] + (self.res[2]/2)
                        if xmid - eps > 320:
                            if prev_direction == DIRECTION_LEFT:
                                if delta_angle < 10:
                                    delta_angle = delta_angle - 1
                                else:
                                    delta_angle = delta_angle/2
                            self.serv_angle = self.serv_angle - delta_angle
                            self.IO.servoSet(self.serv_angle)
                            self.sleep_servo(delta_angle)
                            prev_direction = DIRECTION_RIGHT
                        elif xmid + eps < 320:
                            if prev_direction == DIRECTION_RIGHT:
                                if delta_angle < 10:
                                    delta_angle = delta_angle - 1
                                else:
                                    delta_angle = delta_angle/2
                            self.serv_angle = self.serv_angle + delta_angle
                            self.IO.servoSet(self.serv_angle)
                            self.sleep_servo(delta_angle)
                            prev_direction = DIRECTION_LEFT
                        else:
                            break
                        if delta_angle <= 3:
                            break

                    det = self.detect_obj_resource()

                self.scan_room = False
                break
            else:
                self.IO.setError("flash", cnt = 2)
        
            self.scan_room = False

        #self.room = self.detect_room()
        #if self.room > 0:
        #    self.known_room = True
        #    self.IO.setStatus("flash", cnt = self.room)

        while OK():
            time.sleep(0.05)

    def follow_wall(self, direction, ut=False, unr=False, ue=True):
        if direction == DIRECTION_LEFT:
            return self.follow_left(ut,unr,ue)
        elif direction == DIRECTION_RIGHT:
            return self.follow_right(ut,unr,ue)
        return -1

    def follow_left(self, ut, unr, ue):

        self.reached_room = False
        nowf = startf = 0
        timef = 5
        lego_right = False

        # make sure IR sensor is on the right
        delta_angle = self.serv_angle
        self.serv_angle = 0
        self.IO.servoSet(self.serv_angle)
        self.sleep_servo(delta_angle)

        # motion
        self.start_motion()

        while True:

            lego_right = False

            if ue and self.reached_room == True:
                nowf = time.time()
                if nowf - startf > timef:
                    self.stop_motion()
                    return EVENT_END

            sensors = self.IO.getSensors()
            left_lsensor = sensors[5]
            right_lsensor = sensors[4]
            front = self.check_front_collision()
            if front == True or left_lsensor < 50 or right_lsensor < 50:
                self.turn(90,'l')
                if ut == True:
                    self.stop_motion()
                    return EVENT_TURN
                continue

            right = self.get_right_sensor()
            if right > IR_HIT_THRESHOLD:
                self.turn(5,'l')
            elif right < IR_VOID_THRESHOLD:
                # let it go a little bit
                time.sleep(OUTSIDE_WALL_DELAY1) # enough to also avoid lego bricks
                self.turn(90,'r')
                # let it go a little bit more
                start_nr = time.time()
                now_nr = time.time()
                delta_nr = 4
                while True:
                    new_right  = self.get_right_sensor()
                    # XXX define 200
                    if new_right > 200:
                        time.sleep(OUTSIDE_WALL_DELAY2)
                        break
                    time.sleep(0.25)
                    now_nr = time.time()
                    if (now_nr - start_nr > delta_nr):
                        lego_right = True
                        break
                new_right = self.get_right_sensor()

                # new room
                if new_right < IR_WALL_THRESHOLD:
                    self.turn(90,'r')
                    # let it go a little bit
                    time.sleep(OUTSIDE_WALL_DELAY1)
                    # flash something
                    self.IO.setStatus("flash", cnt = 1)
                    self.IO.setError("flash", cnt = 1)
                    # update_room "left"
                    if self.room == 1:
                        self.room = 6
                    else:
                        self.room = self.room - 1
                    if unr == True:
                        self.stop_motion()
                        return EVENT_NR
                    # if self.room == self.dest_room: stop
                    self.reached_room = True
                    startf = time.time()
                else:
                    if ut == True:
                        self.stop_motion()
                        return EVENT_TURN

            elif right < IR_WALL_THRESHOLD:
                self.turn(5,'r')

    def follow_right(self, ut, unr, ue):

        self.reached_room = False
        nowf = startf = 0
        timef = 5
        lego_left = False

        # make sure IR sensor is on the left
        delta_angle = abs(170-self.serv_angle)
        self.serv_angle = 170
        self.IO.servoSet(self.serv_angle)
        self.sleep_servo(delta_angle)

        # motion
        self.start_motion()
        
        while True:

            lego_left = False

            if ue and self.reached_room == True:
                nowf = time.time()
                if nowf - startf > timef:
                    self.stop_motion()
                    return EVENT_END

            sensors = self.IO.getSensors()
            left_lsensor = sensors[5]
            right_lsensor = sensors[4]

            front = self.check_front_collision()
            if front == True or left_lsensor < 50 or right_lsensor < 50:
                self.turn(90,'r')
                if ut == True:
                    self.stop_motion()
                    return EVENT_TURN
                continue

            left = self.get_left_sensor()
            if left > IR_HIT_THRESHOLD_LEFT:
                self.turn(5,'r')
            elif left < IR_VOID_THRESHOLD:
                time.sleep(OUTSIDE_WALL_DELAY1)
                self.turn(90,'l')
                start_nr = time.time()
                now_nr = time.time()
                delta_nr = 4
                while True:
                    new_left = self.get_left_sensor()
                    if new_left > 140:
                        time.sleep(OUTSIDE_WALL_DELAY2_LEFT)
                        break
                    time.sleep(0.25)
                    now_nr = time.time()
                    if (now_nr - start_nr > delta_nr):
                        lego_left = True
                        break
                new_left = self.get_left_sensor()

                if new_left < IR_WALL_THRESHOLD_LEFT:
                    self.turn(90,'l')
                    # let it go a little bit
                    time.sleep(OUTSIDE_WALL_DELAY1)
                    # flash something
                    self.IO.setStatus("flash", cnt = 1)
                    self.IO.setError("flash", cnt = 1)
                    # update_room "right"
                    if self.room == 6:
                        self.room = 1
                    else:
                        self.room = self.room + 1
                    if unr == True:
                        self.stop_motion()
                        return EVENT_NR
                    self.reached_room = True
                    startf = time.time()
                else:
                    if ut == True:
                        self.stop_motion()
                        return EVENT_TURN
            elif left < IR_WALL_THRESHOLD_LEFT:
                self.turn(5,'l')


    def detect_obj_resource(self):
        if self.res_type == MARIO_RES:
            return self.detect_resource(wf = False,ff = False,zf = False,once = True)
        if self.res_type == WARIO_RES:
            return self.detect_resource(mf = False,ff = False,zf = False,once = True)
        if self.res_type == FRY_RES:
            return self.detect_resource(wf = False,mf = False,zf = False,once = True)
        if self.res_type == ZOIDBERG_RES:
            return self.detect_resource(wf = False,ff = False,mf = False,once = True)
        return False
        

    def detect_resource(self, mf=True, wf=True, ff=True, zf=True, once=False):
        
        total_turn = 0
        delta_turn = 20

        mario_found = False
        wario_found = False
        fry_found = False
        zoidberg_found = False

        while True:

            # grab image
            for i in range(0,10):
                self.IO.cameraGrab()
            image = self.IO.cameraRead()
            image_gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
            
            # detect mario
            if mf:
                marios = self.mario_cascade.detectMultiScale(image_gray, 1.1,\
                        MARIOK, 0, (10,10), (800,800))
                if len(marios) > 0:
                    print "Looking for " + self.res_str + ", recognised Mario, ",
                    # "Looking for X, pick up successful, stopping"
                    mario_found = True
                    res = marios[0]
                    pt1 = (res[0], res[1])
                    pt2 = (res[0] + res[2], res[1] + res[3])
                    if self.res_type == MARIO_RES:
                        print "attempting to pick up\n"
                        self.res = marios[0]
                        return True
                    else:
                        print "continuing exploration\n"

            # detect wario
            if wf:
                warios = self.wario_cascade.detectMultiScale(image_gray, 1.1,\
                                                             WARIOK, 0, (20,20), (800,800))
                if len(warios) > 0:
                    print "Looking for " + self.res_str + ", recognised Wario, ",
                    wario_found = True
                    res = warios[0]
                    pt1 = (res[0], res[1])
                    pt2 = (res[0] + res[2], res[1] + res[3])
                    if self.res_type == WARIO_RES:
                        print "attempting to pick up\n"
                        self.res = warios[0]
                        return True
                    else:
                        print "continuing exploration\n"

            # detect fry
            if ff:
                frys = self.fry_cascade.detectMultiScale(image_gray, 1.1,\
                    FRYK, 0, (10,10), (800,800))
                if len(frys) > 0:
                    print "Looking for " + self.res_str + ", recognised Fry, ",
                    fry_found = True
                    res = frys[0]
                    pt1 = (res[0], res[1])
                    pt2 = (res[0] + res[2], res[1] + res[3])
                    if self.res_type == FRY_RES:
                        print "attempting to pick up\n"
                        self.res = frys[0]
                        return True
                    else:
                        print "continuing exploration\n"

            # detect zoidberg
            if zf:
                zoidbergs = self.zoidberg_cascade.detectMultiScale(image_gray,\
                            1.1, ZOIDBERGK+2, 0, (30,30), (800,800))  # 15
                if len(zoidbergs) > 0:
                    print "Looking for " + self.res_str + ", recognised Zoidberg, ",
                    zoidberg_found = True
                    res = zoidbergs[0]
                    pt1 = (res[0], res[1])
                    pt2 = (res[0] + res[2], res[1] + res[3])
                    if self.res_type == ZOIDBERG_RES:
                        print "attempting to pick up\n"
                        self.res = zoidbergs[0]
                        return True
                    else:
                        print "continuing exploration\n"

            if once == True:
                return False

            # turn camera
            self.serv_angle = self.serv_angle + delta_turn
            total_turn = total_turn + delta_turn

            if total_turn > 360:
                self.turn(200,'r')
                return False

            if self.serv_angle > 180:
                self.turn(200,'l')
                aux_angle = 200 - self.serv_angle
                self.serv_angle = 0
                self.IO.servoSet(self.serv_angle)
                self.sleep_servo(aux_angle)
                continue

            self.IO.servoSet(self.serv_angle)
            self.sleep_servo(delta_turn)

    def trap_resource(self):
        
        # motion
        self.start_motion()
        
        delta_time = 1
        
        while True:

            # wait
            time.sleep(delta_time)

            # check for front
            lfront = self.get_front_lsensor()
            if lfront < 50:
                self.stop_motion()
                print "Grabbed resource\n"
                return

            sensors = self.IO.getSensors()
            inputs = self.IO.getInputs()
            left_switch = inputs[0]
            right_switch = inputs[1]
            left_lsensor = sensors[5]
            right_lsensor = sensors[4]

            if left_switch or left_lsensor < 50:
                # turn left, move until threshold
                self.turn(30,'l')
                while True:
                    fls = self.get_front_lsensor()
                    fr = self.check_front_collision()
                    if fls < 50 or fr == True:
                        self.stop_motion()
                        print "Grabbed resource\n"
                        return

            if right_switch or right_lsensor < 50:
                # turn right, move until threshold
                self.turn(30,'r')
                while True:
                    fls = self.get_front_lsensor()
                    fr = self.check_front_collision()
                    if fls < 50 or fr == True:
                        self.stop_motion()
                        print "Grabbed resource\n"
                        return

            #front = self.check_front_collision()
            #if front == True:
            #    self.stop_motion()
            #    print "Touched something\n"
            #    return

            # check for deviation
            #det = self.detect_obj_resource()
            #if det:
            #    delta_angle = 10
            #    tmp_da = 0
            #    prev_direction = -1
            #    eps = 10
            #    while True:
            #        if not det:
            #            if delta_angle < 10:
            #                delta_angle = delta_angle - 1
            #            else:
            #                delta_angle = delta_angle/2
            #            if prev_direction == DIRECTION_RIGHT:
            #                print "Nothing detected, turning left by %d" % (delta_angle)
            #                self.serv_angle = self.serv_angle + delta_angle
            #                self.IO.servoSet(self.serv_angle)
            #                self.sleep_servo(delta_angle)
            #                prev_direction = DIRECTION_LEFT
            #            elif prev_direction == DIRECTION_LEFT:
            #                print "Nothing detected, turning right by %d" % (delta_angle)
            #                self.serv_angle = self.serv_angle - delta_angle
            #                self.IO.servoSet(self.serv_angle)
            #                self.sleep_servo(delta_angle)
            #                prev_direction = DIRECTION_RIGHT
            #        else:
            #           xmid = self.res[0] + (self.res[2]/2)
            #            print "xmid = %d" % (xmid)
            #            if xmid - eps > 320:
            #                print "Deviation to the right, turning left\n"
            #                if prev_direction == DIRECTION_LEFT:
            #                    if delta_angle < 10:
            #                        delta_angle = delta_angle - 1
            #                    else:
            #                        delta_angle = delta_angle/2
            #                print "delta_angle = %d" % (delta_angle)
            #                self.serv_angle = self.serv_angle - delta_angle
            #                self.IO.servoSet(self.serv_angle)
            #                self.sleep_servo(delta_angle)
            #                prev_direction = DIRECTION_RIGHT
            #            elif xmid + eps < 320:
            #                print "Deviation to the left, turning right\n"
            #                if prev_direction == DIRECTION_RIGHT:
            #                    if delta_angle < 10:
            #                        delta_angle = delta_angle - 1
            #                    else:
            #                        delta_angle = delta_angle/2
            #                print "delta_angle = %d" % (delta_angle)
            #                self.serv_angle = self.serv_angle + delta_angle
            #                self.IO.servoSet(self.serv_angle)
            #                self.sleep_servo(delta_angle)
            #                prev_direction = DIRECTION_LEFT
            #            else:
            #                print "No more deviation, going straight\n"
            #                break
            #            if delta_angle <= 3:
            #                print "Almost no more deviation, going straight\n"
            #                break
            #        det = self.detect_obj_resource()
            #
            #    if self.serv_angle < SERVO_ANGLE_FRONT:
            #        self.turn(SERVO_ANGLE_FRONT-self.serv_angle,'r')
            #        tmp_da = SERVO_ANGLE_FRONT - self.serv_angle
            #    else:
            #        self.turn(self.serv_angle-SERVO_ANGLE_FRONT,'l')
            #        tmp_da = self.serv_angle - SERVO_ANGLE_FRONT
            #    self.serv_angle = SERVO_ANGLE_FRONT
            #    self.IO.servoSet(self.serv_angle)
            #    self.sleep_servo(tmp_da)
            #self.start_motion()

    def detect_room(self):
        
        # sanity var
        total_turn = 0
        delta_turn = 20
        black_flag = yellow_flag = green_flag = blue_flag = False

        while True:
            
            # grab image
            for i in range(0,CAMERA_DELAY):
                self.IO.cameraGrab()

            image = self.IO.cameraRead()
            image_hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

            # white = room 5 (C)
            mask = cv2.inRange(image_hsv, self.lower_white, self.upper_white)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > WHITE_THRESHOLD:
                return 5

            # red = room 6 (E)
            mask = cv2.inRange(image_hsv, self.lower_red, self.upper_red)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > RED_THRESHOLD:
                return 6

            # orange = room 3 (B)
            mask = cv2.inRange(image_hsv, self.lower_orange, self.upper_orange)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > ORANGE_THRESHOLD:
                return 3

            # black
            mask = cv2.inRange(image_hsv, self.lower_black, self.upper_black)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > BLACK_THRESHOLD:
                if yellow_flag:
                    return 2  # (D)
                if not black_flag:
                    black_flag = True

            # yellow
            mask = cv2.inRange(image_hsv, self.lower_yellow, self.upper_yellow)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > YELLOW_THRESHOLD:
                if black_flag:
                    return 2  # (D)
                if green_flag:
                    return 3  # (B)
                if blue_flag:
                    return 2  # (D)
                if not yellow_flag:
                    yellow_flag = True

            # green
            mask = cv2.inRange(image_hsv, self.lower_green, self.upper_green)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > GREEN_THRESHOLD:
                if yellow_flag:
                    return 3  # (B)
                if not green_flag:
                    green_flag = True

            # blue = room 4
            mask = cv2.inRange(image_hsv, self.lower_blue, self.upper_blue)
            positive_pixels = int(numpy.sum(mask))/255
            if positive_pixels > BLUE_THRESHOLD:
                if not blue_flag:
                    blue_flag = True

            # turn camera
            self.serv_angle = self.serv_angle + delta_turn
            total_turn = total_turn + delta_turn

            if total_turn > 360:
                self.IO.setError("flash", cnt = 2)
                print "Couldn't find room"
                total_turn = total_turn - 360
                break

            if self.serv_angle > 180:
                self.turn(220,'r')
                self.serv_angle = 0
                delta_turn = delta_turn + 40

            self.IO.servoSet(self.serv_angle)
            self.sleep_servo(delta_turn)

        # extra checks (in case of errors)
        if yellow_flag:
            return 3  # (B)
        if black_flag:
            return 6  # (E)

        if green_flag:
            return 1  # (F)
        if blue_flag:
            return 4  # (A)

    def get_to_wall(self):

        # find wall
        delta_angle = self.serv_angle
        self.serv_angle = 0
        self.IO.servoSet(self.serv_angle)
        self.sleep_servo(delta_angle)

        sensors = self.IO.getSensors()
        front = self.get_front_sensor(sensors)
        right = self.get_right_sensor(sensors)
        left = self.get_left_sensor(sensors)

        if right > front:
            if right > left:
                front = right
                self.turn(90,'r')
            else:
                front = left
                self.turn(90,'l')
        else:
            if left > front:
                front = left
                self.turn(90,'l')

        # reach wall 
        if front > 330:
            return

        # start moving straight
        self.set_left_motor(LEFT_STRAIGHT)
        self.set_right_motor(RIGHT_STRAIGHT)
        self.update_motors()

        while True:
            front = self.check_front_collision()
            if front == True:
                self.set_left_motor(0)
                self.set_right_motor(0)
                self.update_motors()
                return

    def align_direction(self, direction):

        if direction not in [DIRECTION_LEFT,DIRECTION_RIGHT]:
            print "Invalid direction in align_direction"
            return

        check_threshold = True
        total_turn = 0

        if direction == DIRECTION_LEFT:
            delta_angle = self.serv_angle
            self.serv_angle = 0
            self.IO.servoSet(self.serv_angle)
            self.sleep_servo(delta_angle)

            # until right sensor is max and above a threshold; avoid inf loops
            right = self.get_right_sensor()
            while True:
                self.turn(5,'l')
                new_right = self.get_right_sensor()
                if new_right <= right:
                    if check_threshold:
                        if right > IR_WALL_THRESHOLD:
                            self.turn(5,'r')
                            return
                    else:
                        self.turn(5,'r')
                        return
                right = new_right
                total_turn = total_turn + 5
                if total_turn > 360:
                    check_threshold = False

        # direction = DIRECTION_RIGHT
        delta_angle = abs(170-self.serv_angle)
        self.serv_angle = 170
        self.IO.servoSet(self.serv_angle)
        self.sleep_servo(delta_angle)

        left = self.get_left_sensor()
        while True:
            self.turn(5,'r')
            new_left = self.get_left_sensor()
            if new_left <= left:
                if check_threshold:
                    if left > 155:
                        self.turn(5,'l')
                        return
                else:
                    self.turn(5,'l')
                    return
            left = new_left
            total_turn = total_turn + 5
            if total_turn > 360:
                check_threshold = False

    def find_direction(self):
        if self.room < 1 or self.dest_room < 1 or\
           self.room > 6 or self.dest_room > 6:
            print "Can't find direction, invalid rooms"
            return -1
        if self.dest_room == self.room:
            return 0
        if self.dest_room > self.room:
            dist = self.dest_room - self.room
            if (6 - dist) < dist:
                return DIRECTION_LEFT
            return DIRECTION_RIGHT
        dist = self.room - self.dest_room
        if (6 - dist) < dist:
            return DIRECTION_RIGHT
        return DIRECTION_LEFT

    def check_front_collision(self, sensors=None, inputs=None):
        if sensors is None:
            sensors = self.IO.getSensors()
        front = self.get_front_sensor(sensors)
        if front > 300:  # IR_HIT_THRESHOLD
            return True
        if inputs is None:
            inputs = self.IO.getInputs()
        left_switch = inputs[0]  # LEFT_SWITCH_POS = 0
        right_switch = inputs[1]  # RIGHT_SWITCH_POS = 1
        if left_switch == True or right_switch == True:
            return True
        return False

    def get_front_lsensor(self, sensors=None):
        if sensors is None:
            sensors = self.IO.getSensors()
        return sensors[LF_POS]

    def get_front_sensor(self, sensors=None):
        if sensors is None:
            sensors = self.IO.getSensors()
        return sensors[IRF_POS]

    def get_right_sensor(self, sensors=None):
        if self.serv_angle != 0:
            delta_angle = self.serv_angle
            self.serv_angle = 0
            self.IO.servoSet(self.serv_angle)
            self.sleep_servo(delta_angle)
            sensors = self.IO.getSensors()
        if sensors is None:
            sensors = self.IO.getSensors()
        return sensors[IRT_POS]

    def get_left_sensor(self, sensors=None):
        if self.serv_angle != 170:
            delta_angle = abs(170-self.serv_angle)
            self.serv_angle = 170
            self.IO.servoSet(self.serv_angle)
            self.sleep_servo(delta_angle)
            sensors = self.IO.getSensors()
        if sensors is None:
            sensors = self.IO.getSensors()
        return sensors[IRT_POS]

    def set_left_motor(self, left):
        if left > 100 or left < -100:
            print "bad motor speed for left motor"
            return
        self.ml = left

    def set_right_motor(self, right):
        if right > 100 or right < -100:
            print "bad motor speed for right motor"
            return
        self.mr = right

    def update_motors(self):
        self.IO.setMotors(self.mr, self.ml)

    def turn(self, angle, dir_s):

        if angle == 0:
            return

        # make sure it's float
        angle_fl = float(angle)

        # compute time needed
        delta_time = (angle_fl/90) * TIME_90

        if dir_s == 'l':
            self.IO.setMotors(RIGHT_STRAIGHT,-LEFT_STRAIGHT)
        elif dir_s == 'r':
            self.IO.setMotors(-RIGHT_STRAIGHT,LEFT_STRAIGHT)
        else:
            print "bad param in turn: %s" % (dir_s)
        
        # wait for the turn
        time.sleep(delta_time)

        # go back to normal
        self.IO.setMotors(self.mr,self.ml)

    def translate_room(self,roomc):
        if roomc == 'A':
            return 4
        if roomc == 'B':
            return 3
        if roomc == 'C':
            return 5
        if roomc == 'D':
            return 2
        if roomc == 'E':
            return 6
        if roomc == 'F':
            return 1

    def start_motion(self):
        self.set_left_motor(LEFT_STRAIGHT)
        self.set_right_motor(RIGHT_STRAIGHT)
        self.update_motors()

    def stop_motion(self):
        self.set_left_motor(0)
        self.set_right_motor(0)
        self.update_motors()

    # debug function - gets a char from keyboard
    def _getch(self):
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    # wait for servo to turn
    def sleep_servo(self, delta_angle):
        if delta_angle == 0:
            return
        angle_fl = float(delta_angle)
        delta_time = (angle_fl/180) * TIME_SERVO_180
        time.sleep(delta_time)
