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
#include <util/exit_codes.h>

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

typedef irept::byte_op_type byte_op_type;

bool duplicated_previous_step(const SSA_stept &ssa_step)
{
  return !(ssa_step.is_assignment() || ssa_step.is_assert() ||
            ssa_step.is_assume() || ssa_step.is_constraint() ||
            ssa_step.is_shared_read() || ssa_step.is_shared_write());
}

exprt get_ssa_expr(const SSA_stept &ssa_step)
{
  return ((ssa_step.is_shared_read() || ssa_step.is_shared_write())
                               ? ssa_step.ssa_lhs
                               : ssa_step.cond_expr);
}

void show_ssa_step_plain(messaget::mstreamt &out, const namespacet &ns, 
  const SSA_stept &ssa_step, const exprt &ssa_expr)
{
  const irep_idt &function_id = ssa_step.source.function_id;
  const std::string ssa_expr_as_string = from_expr(ns, function_id, ssa_expr);

  out << messaget::faint << "// " << ssa_step.source.pc->location_number << " ";
  out << ssa_step.source.pc->source_location.as_string() << "\n" << messaget::reset;
  out << ssa_expr_as_string  << "\n";
}

void show_ssa_step_json(std::ostream &out, const namespacet &ns, 
  const SSA_stept &ssa_step, const exprt &ssa_expr, json_arrayt &byte_op_list)
{
  const std::string key_srcloc = "sourceLocation";
  const std::string key_ssa_expr = "ssaExpr";
  const std::string key_ssa_expr_as_string = "ssaExprString";

  json_objectt &json_ssa_step = byte_op_list.push_back(jsont()).make_object();

  json_ssa_step[key_srcloc] = json(ssa_step.source.pc->source_location);

  const irep_idt &function_id = ssa_step.source.function_id;
  const std::string ssa_expr_as_string = from_expr(ns, function_id, ssa_expr);
  json_ssa_step[key_ssa_expr_as_string] = json_stringt(ssa_expr_as_string);
  json_ssa_step[key_ssa_expr] = json_irept(false).convert_from_irep(ssa_expr);
}

void show_byte_op_plain(messaget::mstreamt &out, const namespacet &ns, 
  const symex_target_equationt &equation, const byte_op_type type)
{
  std::size_t equation_byte_op_count = 0;
  for(const auto &step : equation.SSA_steps)
  {
    if(duplicated_previous_step(step))
      continue;

    const exprt &ssa_expr = get_ssa_expr(step);
    const std::size_t ssa_expr_byte_op_count = ssa_expr.get_byte_op_count(type);

    if(ssa_expr_byte_op_count == 0)
      continue;

    equation_byte_op_count += ssa_expr_byte_op_count;
    show_ssa_step_plain(out, ns, step, ssa_expr);
  }

  switch(type)
  {
  case byte_op_type::BYTE_EXTRACT:
    out << '\n' << "Number of byte extracts: ";
    break;
  case byte_op_type::BYTE_UPDATE:
    out << '\n' << "Number of byte updates: ";
    break;
  }

  out << equation_byte_op_count << '\n';
  out << messaget::eom;
}

std::string json_get_key_byte_op_stats(const byte_op_type type)
{
  switch(type)
  {
  case byte_op_type::BYTE_EXTRACT:
    return "byteExtractStats";
  case byte_op_type::BYTE_UPDATE:
    return "byteUpdateStats";
  }
}

std::string json_get_key_byte_op_list(const byte_op_type type)
{
  switch(type)
  {
  case byte_op_type::BYTE_EXTRACT:
    return "byteExtractList";
  case byte_op_type::BYTE_UPDATE:
    return "byteUpdateList";
  }
}

std::string json_get_key_byte_op_num(const byte_op_type type)
{
  switch(type)
  {
  case byte_op_type::BYTE_EXTRACT:
    return "numOfExtracts";
  case byte_op_type::BYTE_UPDATE:
    return "numOfUpdates";
  }
}

void show_byte_op_json(std::ostream &out, const namespacet &ns, 
  const symex_target_equationt &equation, const byte_op_type type, 
  json_objectt &byte_ops_stats)
{
  // Get key values to be used in the json output based on byte operation type
  // 1. json_get_key_byte_op_stats(byte_op_type): returns relevant json object key string
  //    where object records statistics for given byte operation.
  // 2. json_get_key_byte_op_list(byte_op_type): returns relevant json object key string
  //    where object records a list of expressions for given byte operation.
  // 3. json_get_key_byte_op_num(byte_op_type): returns relevant json object key string
  //    where object number of given byte operation.

  const std::string key_byte_op_stats = json_get_key_byte_op_stats(type);
  const std::string key_byte_op_list = json_get_key_byte_op_list(type);
  const std::string key_byte_op_num = json_get_key_byte_op_num(type);

  json_objectt &byte_op_stats = byte_ops_stats[key_byte_op_stats].make_object();
  json_arrayt &byte_op_list = byte_op_stats[key_byte_op_list].make_array();

  std::size_t equation_byte_op_count = 0;
  for(const auto &step : equation.SSA_steps)
  {
    if(duplicated_previous_step(step))
      continue;

    const exprt &ssa_expr = get_ssa_expr(step);
    const std::size_t ssa_expr_byte_op_count = ssa_expr.get_byte_op_count(type);

    if(ssa_expr_byte_op_count == 0)
      continue;

    equation_byte_op_count += ssa_expr_byte_op_count;
    show_ssa_step_json(out, ns, step, ssa_expr, byte_op_list);
  }

  byte_op_stats[key_byte_op_num] = json_numbert(std::to_string(equation_byte_op_count));
}

bool is_outfile_specified(const optionst &options)
{
  const std::string &filename = options.get_option("outfile");
  return(!filename.empty() && filename != "-");
}

void show_byte_ops(const optionst &options, 
  ui_message_handlert &ui_message_handler, 
  const namespacet &ns, const symex_target_equationt &equation)
{
  const std::string &filename = options.get_option("outfile");
  const bool outfile_given = is_outfile_specified(options);

  std::ofstream of;

  if(outfile_given)
  {
    of.open(filename,std::fstream::out);
    if(!of)
      throw invalid_command_line_argument_exceptiont(
        "failed to open output file: " + filename, "--outfile");
  }

  std::ostream &out = outfile_given ? of : std::cout;

  messaget msg(ui_message_handler);
  switch(ui_message_handler.get_ui())
  {
  case ui_message_handlert::uit::XML_UI:
    msg.error() << "XML UI not supported for displaying byte extracts and updates" << "\n"
                << "Try --json-ui instead" << messaget::eom;
    exit(CPROVER_EXIT_USAGE_ERROR);

  case ui_message_handlert::uit::JSON_UI:
  {
    json_objectt json_result;
    json_objectt &byte_ops_stats = json_result["byteOpsStats"].make_object();

    show_byte_op_json(out, ns, equation, byte_op_type::BYTE_EXTRACT, byte_ops_stats);
    show_byte_op_json(out, ns, equation, byte_op_type::BYTE_UPDATE, byte_ops_stats);

    out << ",\n" << json_result;
    break;
  }

  case ui_message_handlert::uit::PLAIN:
    if(outfile_given)
    {
      stream_message_handlert mout_handler(out);
      messaget mout(mout_handler);

      msg.status() << "\nByte Extracts written to file"
                   << messaget::eom;
      show_byte_op_plain(mout.status(), ns, equation, byte_op_type::BYTE_EXTRACT);

      msg.status() << "\nByte Updates written to file"
                   << messaget::eom;
      show_byte_op_plain(mout.status(), ns, equation, byte_op_type::BYTE_UPDATE);
    }
    else
    {
      msg.status() << "\nByte Extracts:" << messaget::eom;
      show_byte_op_plain(msg.status(), ns, equation, byte_op_type::BYTE_EXTRACT);

      msg.status() << "\nByte Updates:" << messaget::eom;
      show_byte_op_plain(msg.status(), ns, equation, byte_op_type::BYTE_UPDATE);
    }
    break;
  }

  if(outfile_given)
    of.close();
}
