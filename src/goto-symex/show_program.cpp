/*******************************************************************\

Module: Output of the program (SSA) constraints

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Output of the program (SSA) constraints

#include "show_program.h"

#include <iostream>
#include <fstream>

#include <goto-symex/symex_target_equation.h>

#include <langapi/language_util.h>

#include <util/ui_message.h>
#include <util/json_irep.h>

/// Output a single SSA step
/// \param ns: Namespace
/// \param step: SSA step
/// \param annotation: Additonal information to include in step output
/// \param count: Step number, incremented after output
static void show_step(
  const namespacet &ns,
  const SSA_stept &step,
  const std::string &annotation,
  std::size_t &count)
{
  const irep_idt &function_id = step.source.function_id;

  std::string string_value = (step.is_shared_read() || step.is_shared_write())
                               ? from_expr(ns, function_id, step.ssa_lhs)
                               : from_expr(ns, function_id, step.cond_expr);
  std::cout << '(' << count << ") ";
  if(annotation.empty())
    std::cout << string_value;
  else
    std::cout << annotation << '(' << string_value << ')';
  std::cout << '\n';

  if(!step.guard.is_true())
  {
    const std::string guard_string = from_expr(ns, function_id, step.guard);
    std::cout << std::string(std::to_string(count).size() + 3, ' ');
    std::cout << "guard: " << guard_string << '\n';
  }

  ++count;
}

void show_program(const namespacet &ns, const symex_target_equationt &equation)
{
  std::size_t count = 1;

  std::cout << '\n' << "Program constraints:" << '\n';

  for(const auto &step : equation.SSA_steps)
  {
    std::cout << "// " << step.source.pc->location_number << " ";
    std::cout << step.source.pc->source_location.as_string() << "\n";

    if(step.is_assignment())
      show_step(ns, step, "", count);
    else if(step.is_assert())
      show_step(ns, step, "ASSERT", count);
    else if(step.is_assume())
      show_step(ns, step, "ASSUME", count);
    else if(step.is_constraint())
    {
      PRECONDITION(step.guard.is_true());
      show_step(ns, step, "CONSTRAINT", count);
    }
    else if(step.is_shared_read())
      show_step(ns, step, "SHARED_READ", count);
    else if(step.is_shared_write())
      show_step(ns, step, "SHARED_WRITE", count);
  }
}

// Byte extracts
static std::size_t get_number_of_byte_extracts(const irept &irep)
{

  std::size_t byte_extract_count = 0;

  if(!irep.id().empty())
  {
    if(irep.id() == ID_byte_extract_little_endian ||
      irep.id() == ID_byte_extract_big_endian)
    {
      byte_extract_count++;
    }
  }

  forall_irep(it, irep.get_sub())
  {
    byte_extract_count += get_number_of_byte_extracts(*it);
  }

  return byte_extract_count;
}

void show_byte_extracts_plain(messaget::mstreamt &out,
  const namespacet &ns, const symex_target_equationt &equation)
{

  std::size_t byte_extract_count = 0;
  std::size_t ssa_byte_extract_count = 0;
  bool proceed = false;

  for(const auto &step : equation.SSA_steps)
  {
    const irep_idt &function_id = step.source.function_id;

    std::string string_value = (step.is_shared_read() || step.is_shared_write())
                               ? from_expr(ns, function_id, step.ssa_lhs)
                               : from_expr(ns, function_id, step.cond_expr);

    const exprt &s_expr = (step.is_shared_read() || step.is_shared_write())
                               ? step.ssa_full_lhs
                               : step.cond_expr;

    proceed = (step.is_assignment() || step.is_assert() ||
            step.is_assume() || step.is_constraint() ||
            step.is_shared_read() || step.is_shared_write());

    if(proceed)
      ssa_byte_extract_count = get_number_of_byte_extracts(s_expr);
    else
      ssa_byte_extract_count = 0;

    if(ssa_byte_extract_count > 0)
    {
      out << messaget::faint << "// " << step.source.pc->location_number << " ";
      out << step.source.pc->source_location.as_string() << "\n" << messaget::reset;
      out << string_value << "\n";
    }

    byte_extract_count += ssa_byte_extract_count;
  }

  out << '\n' << "Number of byte extracts: " << byte_extract_count << '\n';

  out << messaget::eom;
}

void show_byte_extracts_json(std::ostream &out,
  const namespacet &ns, const symex_target_equationt &equation)
{

  std::size_t byte_extract_count = 0;
  std::size_t ssa_byte_extract_count = 0;
  bool proceed = false;

  json_objectt json_result;

  json_arrayt &json_b_extract = json_result["byte_extract"].make_array();

  for(const auto &step : equation.SSA_steps)
  {
    const irep_idt &function_id = step.source.function_id;

    std::string string_value = (step.is_shared_read() || step.is_shared_write())
                               ? from_expr(ns, function_id, step.ssa_lhs)
                               : from_expr(ns, function_id, step.cond_expr);

    const exprt &s_expr = (step.is_shared_read() || step.is_shared_write())
                               ? step.ssa_full_lhs
                               : step.cond_expr;

    proceed = (step.is_assignment() || step.is_assert() ||
            step.is_assume() || step.is_constraint() ||
            step.is_shared_read() || step.is_shared_write());

    if(proceed)
      ssa_byte_extract_count = get_number_of_byte_extracts(s_expr);
    else
      ssa_byte_extract_count = 0;

    if(ssa_byte_extract_count > 0)
    {
      //byte extract object
      json_objectt &object = json_b_extract.push_back(jsont()).make_object();

      object["sourceLocation"] = json(step.source.pc->source_location);
      object["ssaExprString"] = json_stringt(string_value);
      object["ssaExpr"] = json_irept(false).convert_from_irep(s_expr);

    }

    byte_extract_count += ssa_byte_extract_count;
  }

  json_result["numOfExtracts"] = json_numbert(std::to_string(byte_extract_count));

  out << ",\n" << json_result;

}

// Display all byte extracts
void show_byte_extracts(const optionst &options,
  ui_message_handlert &ui_message_handler,
  const namespacet &ns, const symex_target_equationt &equation)
{

  messaget msg(ui_message_handler);

  const std::string &filename = options.get_option("outfile");
  bool have_file = !filename.empty() && filename != "-";

  std::ofstream of;

  if(have_file)
  {
    of.open(filename);
    if(!of)
      throw invalid_command_line_argument_exceptiont(
        "failed to open output file: " + filename, "--outfile");
  }

  std::ostream &out = have_file ? of : std::cout;

  switch(ui_message_handler.get_ui())
  {
  case ui_message_handlert::uit::XML_UI:
    msg.error() << "XML UI not supported" << messaget::eom;
    return;

  case ui_message_handlert::uit::JSON_UI:
    show_byte_extracts_json(out, ns, equation);
    break;

  case ui_message_handlert::uit::PLAIN:
    if(have_file)
    {
      msg.status() << "\nByte extracts written to file"
                   << messaget::eom;
      stream_message_handlert mout_handler(out);
      messaget mout(mout_handler);
      show_byte_extracts_plain(mout.status(), ns, equation);
    }
    else
    {
      msg.status() << "\nByte Extracts:" << messaget::eom;
      show_byte_extracts_plain(msg.status(), ns, equation);
    }
    break;
  }

  if(have_file)
    of.close();
}
