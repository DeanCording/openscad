/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2011 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "offsetextrudenode.h"

#include "module.h"
#include "ModuleInstantiation.h"
#include "fileutils.h"
#include "builtin.h"
#include "polyset.h"
#include "children.h"
#include "parameters.h"

#include <sstream>
#include "boost-utils.h"
#include <boost/assign/std/vector.hpp>
using namespace boost::assign; // bring 'operator+=()' into scope

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

static AbstractNode* builtin_offset_extrude(const ModuleInstantiation *inst, Arguments arguments, Children children)
{

	auto node = new OffsetExtrudeNode(inst);
	// if height not given, and first argument is a number,
	// then assume it should be the height.
	bool first_argument_is_height = (arguments.size() > 0 && !arguments[0].name && arguments[0]->type() == Value::Type::NUMBER);
	Parameters parameters = first_argument_is_height ?
		Parameters::parse(std::move(arguments), inst->location(),
			{"height", "r", "delta", "chamfer", "center", "slices"},
			{"convexity"}
		)
	:
		Parameters::parse(std::move(arguments), inst->location(),
			{"r", "delta", "height", "chamfer", "center",  "slices"},
			{"convexity"}
		)
	;

	node->fn = parameters["$fn"].toDouble();
	node->fs = parameters["$fs"].toDouble();
	node->fa = parameters["$fa"].toDouble();


	node->height = 1;
    if (parameters["height"].isDefined()) {
		parameters["height"].getFiniteDouble(node->height);
	}
	
    double tmp_convexity = 0.0;
    parameters["convexity"].getFiniteDouble(tmp_convexity);
	node->convexity = static_cast<int>(tmp_convexity);
    
	if (node->height == 0) node->height = 1;

	if (node->convexity <= 0)
		node->convexity = 1;

	double slicesVal = 0;
	parameters["slices"].getFiniteDouble(slicesVal);
    node->slices = static_cast<int>(slicesVal);
	node->slices = std::max(node->slices, 1);

	if (parameters["r"].isDefinedAs(Value::Type::NUMBER)) {
		parameters["r"].getDouble(node->delta);
	} else if (parameters["delta"].isDefinedAs(Value::Type::NUMBER)) {
		parameters["delta"].getDouble(node->delta);
		node->join_type = ClipperLib::jtMiter;
		if (parameters["chamfer"].isDefinedAs(Value::Type::BOOL) && parameters["chamfer"].toBool()) {
			node->chamfer = true;
			node->join_type = ClipperLib::jtSquare;
		}
	}

	if (parameters["center"].type() == Value::Type::BOOL)
		node->center = parameters["center"].toBool();

    children.instantiate(node);
	
	return node;
}

std::string OffsetExtrudeNode::toString() const
{
	std::stringstream stream;

	bool isRadius = this->join_type == ClipperLib::jtRound;
	auto var = isRadius ? "(r = " : "(delta = ";

	stream  << this->name() << var << std::dec << this->delta;
	if (!isRadius) {
		stream << ", chamfer = " << (this->chamfer ? "true" : "false");
	}
	stream << ", height = " << this->height
				 << ", slices = " << this->slices
				 << ", center = " << (this->center ? "true" : "false")
				 << ", $fn = " << this->fn
				 << ", $fa = " << this->fa
				 << ", $fs = " << this->fs << ")";

	return stream.str();
}

void register_builtin_offset_extrude()
{
	Builtins::init("offset_extrude", new BuiltinModule(builtin_offset_extrude), {
			"offset_extrude(height, r = 1, slices = 1, center = false[, $fn, $fa, $fs])",
			"offset_extrude(height, delta = 1, slices = 1, chamfer = false, center = false[, $fn, $fa, $fs])",
	});
}
