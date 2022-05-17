#include "CodeFormatServer/Service/CodeFormatService.h"
#include "CodeService/LuaFormatter.h"
#include "CodeService/FormatElement/DiagnosisContext.h"
#include "CodeService/NameStyle/NameStyleChecker.h"

CodeFormatService::CodeFormatService(std::shared_ptr<LanguageClient> owner)
	: Service(owner),
	  _spellChecker(std::make_shared<CodeSpellChecker>())
{
}

std::vector<vscode::Diagnostic> CodeFormatService::Diagnose(std::string_view filePath,
                                                            std::shared_ptr<LuaParser> parser,
                                                            std::shared_ptr<LuaCodeStyleOptions> options)
{
	std::vector<vscode::Diagnostic> diagnostics;

	LuaFormatter formatter(parser, *options);
	formatter.BuildFormattedElement();
	DiagnosisContext ctx(parser, *options);
	formatter.CalculateDiagnosisInfos(ctx);

	if (options->enable_check_codestyle)
	{
		NameStyleChecker styleChecker(ctx);
		styleChecker.Analysis();
	}

	_spellChecker->Analysis(ctx);

	auto diagnosisInfos = ctx.GetDiagnosisInfos();
	for (auto diagnosisInfo : diagnosisInfos)
	{
		auto& diagnosis = diagnostics.emplace_back();
		diagnosis.message = diagnosisInfo.Message;
		diagnosis.range = vscode::Range(
			vscode::Position(
				diagnosisInfo.Range.Start.Line,
				diagnosisInfo.Range.Start.Character
			),
			vscode::Position(
				diagnosisInfo.Range.End.Line,
				diagnosisInfo.Range.End.Character
			));
		diagnosis.severity = vscode::DiagnosticSeverity::Warning;
		switch (diagnosisInfo.type)
		{
		case DiagnosisType::Indent:
		case DiagnosisType::Blank:
		case DiagnosisType::Align:
			{
				diagnosis.data = "emmylua.format";
				break;
			}
		case DiagnosisType::Spell:
			{
				diagnosis.severity = vscode::DiagnosticSeverity::Information;
				diagnosis.data = "emmylua.spell|" + diagnosisInfo.Data;
				break;
			}
		default:
			{
				break;
			}
		}
	}

	return diagnostics;
}

std::string CodeFormatService::Format(std::shared_ptr<LuaParser> parser, std::shared_ptr<LuaCodeStyleOptions> options)
{
	LuaFormatter formatter(parser, *options);
	formatter.BuildFormattedElement();

	return formatter.GetFormattedText();
}

std::string CodeFormatService::RangeFormat(LuaFormatRange& range,
                                           std::shared_ptr<LuaParser> parser,
                                           std::shared_ptr<LuaCodeStyleOptions> options)
{
	LuaFormatter formatter(parser, *options);
	formatter.BuildFormattedElement();
	return formatter.GetRangeFormattedText(range);
}

void CodeFormatService::MakeSpellActions(std::shared_ptr<vscode::CodeActionResult> result,
                                         vscode::Diagnostic& diagnostic, std::string_view uri)
{
	auto pos = diagnostic.data.find_first_of("|");
	if (pos == std::string::npos)
	{
		return;
	}

	auto originText = diagnostic.data.substr(pos + 1);
	if (originText.empty())
	{
		return;
	}

	auto letterWord = originText;
	for (auto& c : letterWord)
	{
		c = ::tolower(c);
	}
	bool upperFirst = false;
	if(std::isupper(originText.front()))
	{
		upperFirst = true;
	}

	auto suggests = _spellChecker->GetSuggests(letterWord);
	for (auto& suggest : suggests)
	{
		if (!suggest.Term.empty()) {
			auto& action = result->actions.emplace_back();
			auto term = suggest.Term;
			if(upperFirst)
			{
				term[0] = std::toupper(term[0]);
			}

			action.title = term;
			action.command.title = term;
			action.command.command = "emmylua.spell.correct";
			action.command.arguments.push_back(uri);
			action.command.arguments.push_back(diagnostic.range.Serialize());
			action.command.arguments.push_back(term);

			action.kind = vscode::CodeActionKind::QuickFix;
		}
	}
}

void CodeFormatService::LoadDictionary(std::string_view path)
{
	_spellChecker->LoadDictionary(path);
}

bool CodeFormatService::IsCodeFormatDiagnostic(vscode::Diagnostic& diagnostic)
{
	return diagnostic.data == "emmylua.format";
}

bool CodeFormatService::IsSpellDiagnostic(vscode::Diagnostic& diagnostic)
{
	return diagnostic.data.starts_with("emmylua.spell");
}
