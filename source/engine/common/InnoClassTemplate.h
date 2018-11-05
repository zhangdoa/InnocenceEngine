#pragma once

#define INNO_POLYMORPHISM_TYPE_CONCRETE ( className ) \
className(void) = default; \
~className(void) = default; \

#define INNO_POLYMORPHISM_TYPE_ABSTRACT ( className ) \
className(void) = default; \
virtual ~className(void) = default; \

#define INNO_ASSIGNMENT_TYPE_DEFALUT( className ) \
className(const className& rhs) = default; \
className& operator=(const className& rhs) = default; \
className(className&& other) = default; \
className& operator=(className&& other) = default; \

#define INNO_ASSIGNMENT_TYPE_COPYABLE( className ) \
className(const className& rhs) = default; \
className& operator=(const className& rhs) = default; \

#define INNO_ASSIGNMENT_TYPE_MOVABLE( className ) \
className(className&& other) = default; \
className& operator=(className&& other) = default; \

#define INNO_ASSIGNMENT_TYPE_NON_COPYABLE( className ) \
className(const className& rhs) = delete; \
className& operator=(const className& rhs) = delete; \

#define INNO_ASSIGNMENT_TYPE_NON_MOVABLE( className ) \
className(className&& other) = delete; \
className& operator=(className&& other) = delete; \