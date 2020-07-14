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

#define BYTE_EXTRACT 0
#define BYTE_UPDATE 1

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

bool is_byte_extract(const irept &irep)
{
  return (irep.id() == ID_byte_extract_little_endian ||
      irep.id() == ID_byte_extract_big_endian);
}

bool is_byte_update(const irept &irep)
{
  return (irep.id() == ID_byte_update_little_endian ||
      irep.id() == ID_byte_update_big_endian);
}

std::size_t get_byte_op_count(const irept &irep, int byte_op_type)
{
  std::size_t count = 0;
  if(!irep.id().empty())
  {
    switch(byte_op_type)
    {
    case BYTE_EXTRACT:
      if(is_byte_extract(irep))
        count += 1;
      break;
    case BYTE_UPDATE:
      if(is_byte_update(irep))
        count += 1;
      break;
    default:
      std::cout << "Usage error!\n";
      exit(CPROVER_EXIT_USAGE_ERROR);
    }
  }

  forall_irep(it, irep.get_sub())
  {
    count += get_byte_op_count(*it, byte_op_type);
  }

  return count; 
}

void show_ssa_step_plain(messaget::mstreamt &out, const namespacet &ns, 
  const SSA_stept &ssa_step, const exprt &ssa_expr)
{
  const irep_idt &function_id = ssa_step.source.function_id;
  std::string string_value = from_expr(ns, function_id, ssa_expr);

  out << messaget::faint << "// " << ssa_step.source.pc->location_number << " ";
  out << ssa_step.source.pc->source_location.as_string() << "\n" << messaget::reset;
  out << string_value << "\n";
}

void show_ssa_step_json(std::ostream &out, const namespacet &ns, 
  const SSA_stept &ssa_step, const exprt &ssa_expr, json_arrayt &byte_op_list)
{
  json_objectt &json_ssa_step = byte_op_list.push_back(jsont()).make_object();

  json_ssa_step["sourceLocation"] = json(ssa_step.source.pc->source_location);

  const irep_idt &function_id = ssa_step.source.function_id;
  std::string string_value = from_expr(ns, function_id, ssa_expr);
  json_ssa_step["ssaExprString"] = json_stringt(string_value);

  json_ssa_step["ssaExpr"] = json_irept(false).convert_from_irep(ssa_expr);
}

void show_byte_op_plain(messaget::mstreamt &out, const namespacet &ns, 
  const symex_target_equationt &equation, const int byte_op_type)
{
  std::size_t equation_byte_op_count = 0;
  for(const auto &step : equation.SSA_steps)
  {
    if(duplicated_previous_step(step))
      continue;

    const exprt &ssa_expr = get_ssa_expr(step);
    std::size_t ssa_expr_byte_op_count = get_byte_op_count(ssa_expr, byte_op_type);

    if(ssa_expr_byte_op_count == 0)
      continue;

    equation_byte_op_count += ssa_expr_byte_op_count;
    show_ssa_step_plain(out, ns, step, ssa_expr);
  }

  switch(byte_op_type)
  {
  case BYTE_EXTRACT:
    out << '\n' << "Number of byte extracts: ";
    break;
  case BYTE_UPDATE:
    out << '\n' << "Number of byte updates: ";
    break;
  default:
    std::cout << "Usage error!\n";
    exit(CPROVER_EXIT_USAGE_ERROR);
  }

  out << equation_byte_op_count << '\n';
  out << messaget::eom;
}

std::string json_get_key_byte_op_stats(int byte_op_type)
{
  switch(byte_op_type)
  {
  case BYTE_EXTRACT:
    return "byteExtractStats";
  case BYTE_UPDATE:
    return "byteUpdateStats";
  default:
    std::cout << "Usage error!\n";
    exit(CPROVER_EXIT_USAGE_ERROR);
  }
}

std::string json_get_key_byte_op_list(int byte_op_type)
{
  switch(byte_op_type)
  {
  case BYTE_EXTRACT:
    return "byteExtractList";
  case BYTE_UPDATE:
    return "byteUpdateList";
  default:
    std::cout << "Usage error!\n";
    exit(CPROVER_EXIT_USAGE_ERROR);
  }
}

std::string json_get_key_byte_op_num(int byte_op_type)
{
  switch(byte_op_type)
  {
  case BYTE_EXTRACT:
    return "numOfExtracts";
  case BYTE_UPDATE:
    return "numOfUpdates";
  default:
    std::cout << "Usage error!\n";
    exit(CPROVER_EXIT_USAGE_ERROR);
  }
}
