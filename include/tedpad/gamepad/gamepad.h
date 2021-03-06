#pragma once

#include <vector>
#include <string>
#include <map>
#include "../../../engine/include/eg/eg_engine.h"

#include "filetype_tpd.h"
#include "../modules/attribute.h"
#include "../modules/gamepadDescription.h"
#include "../modules/gamepadData.h"

//TOOD: Work out something to remove all the repetition in gamepad.cpp (probably need to change something in attribute)
//TODO: Add some add_attribute member functions that don't require a pointer to a generic attribute
//TODO: Clean up the mess that occurs between AttributeType and Module::Attribute::PlaceHolderType (eg some functions use AttributeType, some use placeholder type)
//TODO: Clean up the same mess except between AttributeDirection and Module::Attribute::Direction
//TODO: Rename IODirection to gamepadLocation or something
//TODO: Learn more about specializing member function templates

namespace tedpad {
	class Gamepad {
		//Will return true if p0 matches the values given in the constructor
		struct p_AttributeMatch {
			p_AttributeMatch(eg::Descriptor<> const description, Module::Attribute::DataDirection const direction, std::string const &name);
			bool operator() (Module::Attribute::Generic const *const p0) const;
			eg::Descriptor<> attr_description;
			Module::Attribute::DataDirection attr_direction;
			std::string attr_name;
		};
		//Returns p0 attribute type
		struct p_AttributeDetermine_PlaceHolderType {
			p_AttributeDetermine_PlaceHolderType(Module::Attribute::Generic const *const p0 = nullptr);
			Module::Attribute::PlaceHolderType operator() (Module::Attribute::Generic const *const p0) const;
			operator Module::Attribute::PlaceHolderType() const;

			Module::Attribute::PlaceHolderType value = Module::Attribute::PlaceHolderType::Digital;
		};
		//Returns p0 attribute type
		struct p_AttributeDetermine_Description {
			Module::Attribute::PlaceHolderType operator() (Module::Attribute::Generic const *const p0) const;
		};
		static std::map<eg::Descriptor<> const, Module::Attribute::PlaceHolderType const> const map_Description_PlaceHolderType;
	public:
		enum class AttributeType : uint8_t {
			Digital,
			Analogue,
			Axis,
			Buffer,
			Invalid,
		};
		enum class AttributeDirection : uint8_t {
			Get,
			Set,
		};
		enum class IODirection : uint8_t {
			Server,
			Client,
		};
		//Get the data of the specified attribute. Returns 0 for success.
		//Get a digital button attribute
		bool Get_attribute(std::string const &attribute, bool &out) const;
		//Get an analogue button attribute
		bool Get_attribute(std::string const &attribute, uint8_t &out) const;
		//Get an axis attribute
		bool Get_attribute(std::string const &attribute, int16_t &out) const;
		//Get a buffer attribute
		bool Get_attribute(std::string const &attribute, std::vector<uint8_t> &out) const;
		//Get the type of an attribute (can be an attribute of either direction)
		AttributeType get_attributeType(std::string const &attribute) const;
		//Set the data of the specified attribute. Returns 0 for success.
		//Set a digital button attribute
		bool Set_attribute(std::string const &attribute, bool const in);
		//Set an analogue button attribute
		bool Set_attribute(std::string const &attribute, uint8_t const in);
		//Set an axis attribute
		bool Set_attribute(std::string const &attribute, int16_t const in);
		//Set a buffer attribute
		bool Set_attribute(std::string const &attribute, std::vector<uint8_t> const &in);

		//Create a gamepad from a .tpd file (should this be vector or string?)
		//This is the contents of the file: not the path
		void from_tpdFile(std::string const &fileContent);
		void from_tpdFile(std::vector<uint8_t> const &fileContent);
		void from_tpdFile(std::vector<char> const &fileContent);
		void from_tpdFile(char const *const fileContent, size_t const len);
		void from_tpdFile(uint8_t const *const fileContent, size_t const len);

		//Allocate attributes and set name to align with p0
		void from_gamepadFullDescription(Module::GamepadFullDescription const &p0);
		//Get a GamepadBreifDescription describing the gamepad
		Module::GamepadBriefDescription to_gamepadBriefDescription() const;
		//Get a GamepadFullDescription describing the gamepad
		Module::GamepadFullDescription to_gamepadFullDescription() const;

		//Set the attribute data for the gamepad (that can be set). If the gamepad is IODirection::Server, it will set the data with direction ServerOut.
		//Has a check to ensure that p0 does not set read/get only attributes
		void set_gamepadData(Module::GamepadData const &p0);
		//Get the attribute data for the gamepad (that can be read). If the gamepad is IODirection::Server, it will get the data with direction ServerIn.
		Module::GamepadData get_gamepadData() const;

		//Same as set_gamepadData, except read only values can also be set
		void set_gamepadData_dataDirection(Module::GamepadData const &p0);
		Module::GamepadData get_gamepadData_dataDirection(Module::Attribute::DataDirection const dataDirection) const;
		
		//Set whether the gamepad is on the server or client side
		void set_IODirection(IODirection const p0);

		//Add an attribute to the gamepad
		template <typename predicate_t = p_AttributeDetermine_Description>
		void add_attribute(Module::Attribute::Generic const *const p0, predicate_t p = predicate_t());

		//Add an attribute to the gamepad
		void add_newAttribute(AttributeType const type, AttributeDirection const direction, std::string const &name);

		Gamepad &operator=(Gamepad const &p0);
		Gamepad &operator=(Gamepad &&p0);
		Gamepad(Gamepad const &p0);
		Gamepad(Gamepad &&p0);

		Gamepad();
		Gamepad(std::string const &name, IODirection ioDirection = IODirection::Server);
		~Gamepad();
	private:
		static std::map<AttributeType const, Module::Attribute::PlaceHolderType const> const map_AttributeType_PlaceHolderType;

		void op_assign(Gamepad const &p0);
		void op_assign(Gamepad &&p0);

		Module::Attribute::DataDirection conv_attributeDirection_to_dataDirection(AttributeDirection const direction) const;
		Module::Attribute::PlaceHolderType conv_attributeType_to_placeHolderType(AttributeType const type) const;
		AttributeType conv_placeHolderType_to_attributeType(Module::Attribute::PlaceHolderType const type) const;
		
		
		uint8_t get_attributeServerOutCount(eg::Descriptor<> const attr_desc) const;
		uint8_t get_attributeServerInCount(eg::Descriptor<> const attr_desc) const;

		void set_attributeItr(std::vector<Module::Attribute::Generic *>::iterator const itr, Module::Attribute::Generic const *const p0);
		void get_attributeItr(std::vector<Module::Attribute::Generic *>::const_iterator const itr, Module::Attribute::Generic *const p0) const;

		IODirection ioDirection = IODirection::Server;
		std::string gamepadName;
		std::vector<Module::Attribute::Generic *> vec_attribute;
	};
}
