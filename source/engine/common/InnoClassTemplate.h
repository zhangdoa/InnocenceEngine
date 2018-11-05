#pragma once

#define INNO_CLASS_CONCRETE_DEFALUT( className ) \
className(void) = default; \
~className(void) = default; \
className(const className& rhs) = default; \
className& operator=(const className& rhs) = default; \
className(className&& other) = default; \
className& operator=(className&& other) = default;

#define INNO_CLASS_CONCRETE_NON_COPYABLE( className ) \
className(void) = default; \
~className(void) = default; \
className(const className& rhs) = delete; \
className& operator=(const className& rhs) = delete; \
className(className&& other) = default; \
className& operator=(className&& other) = default;

#define INNO_CLASS_CONCRETE_NON_MOVABLE( className ) \
className(void) = default; \
~className(void) = default; \
className(const className& rhs) = default; \
className& operator=(const className& rhs) = default; \
className(className&& other) = delete; \
className& operator=(className&& other) = delete;

#define INNO_CLASS_CONCRETE_NON_COPYABLE_AND_NON_MOVABLE( className ) \
className(void) = default; \
~className(void) = default; \
className(const className& rhs) = delete; \
className& operator=(const className& rhs) = delete; \
className(className&& other) = delete; \
className& operator=(className&& other) = delete;

#define INNO_CLASS_INTERFACE_DEFALUT( className ) \
className(void) = default; \
virtual ~className(void) = default; \
className(const className& rhs) = default; \
className& operator=(const className& rhs) = default; \
className(className&& other) = default; \
className& operator=(className&& other) = default;

#define INNO_CLASS_INTERFACE_NON_COPYABLE( className ) \
className(void) = default; \
virtual ~className(void) = default; \
className(const className& rhs) = delete; \
className& operator=(const className& rhs) = delete; \
className(className&& other) = default; \
className& operator=(className&& other) = default;

#define INNO_CLASS_INTERFACE_NON_MOVABLE( className ) \
className(void) = default; \
virtual ~className(void) = default; \
className(const className& rhs) = default; \
className& operator=(const className& rhs) = default; \
className(className&& other) = delete; \
className& operator=(className&& other) = delete;

#define INNO_CLASS_INTERFACE_NON_COPYABLE_AND_NON_MOVABLE( className ) \
className(void) = default; \
virtual ~className(void) = default; \
className(const className& rhs) = delete; \
className& operator=(const className& rhs) = delete; \
className(className&& other) = delete; \
className& operator=(className&& other) = delete;