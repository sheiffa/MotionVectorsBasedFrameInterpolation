#!/usr/bin/env python

from PIL import*
import Image

class Motion:
	def __init__(self,points):
		self.points=points
	def generatePoints(function, range):
		self.points=()
		for i in range(range):
			self.points+=(i,function(i))

class Scene:
	def __init__(self,w,h,motion):
		self.w=w
		self.h=h
		self.motion=motion

width=400
height=400
linearMotion=Motion()
linearMotion.generatePoints((lambda x: x),width)
scene=Scene(width, heigth, linearMotion)