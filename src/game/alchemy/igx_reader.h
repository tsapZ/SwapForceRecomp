#pragma once

// Reader for the game's .igx files: Alchemy objects written out as plain XML, e.g. the toy data
//
//   <object refname="ChopChop_ToyData" type="CFullCharacterToyData" root="true">
//     <var name="_toyId" value="kTfbSpyroTag_ToyType_Character_ChopChop"/>
//     <var name="_variants" ref="this.ChopChop_ToyData_variants"/>
//   </object>
//   <object refname="ChopChop_ToyData_variants" type="CVariantIdentifierList" count="3">
//     <listitem ref="this.ChopChop_Repose2012_variantIdentifier"/>
//   </object>
//
// Only this subset is understood: objects with vars (value or ref) and list items.

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace swapforcerecomp::alchemy {

struct IgxObject {
  std::string refname;
  std::string type;
  std::unordered_map<std::string, std::string> vars;  // name -> value or ref
  std::vector<std::string> items;                      // list item refs ("this." stripped)

  // The var's value, or empty.
  const std::string& Var(const std::string& name) const;
};

std::vector<IgxObject> ParseIgx(std::string_view text);

}  // namespace swapforcerecomp::alchemy
