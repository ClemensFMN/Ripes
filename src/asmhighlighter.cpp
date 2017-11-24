#include "asmhighlighter.h"

#include <QList>
#include <QRegularExpressionMatchIterator>

#include "defines.h"

FieldType::FieldType(Type type, int lowerBound, int upperBound) {
    m_type = type;
    m_lowerBound = lowerBound;
    m_upperBound = upperBound;
}

QString FieldType::validateField(const QString& field) const {
    const static auto registerRegex = QRegularExpression("\\b[(a|s|t|x)][0-9]{1,2}");

    switch (m_type) {
        case Type::Immediate: {
            // Check that immediate can be converted
            bool ok = false;
            auto immediate = field.toInt(&ok, 10);
            if (!ok) {
                return QString("Invalid immediate field - got %1").arg(field);
            }
            // Check that immediate is within range
            if (immediate >= m_lowerBound && immediate <= m_upperBound) {
                return QString();
            } else {
                return QString(
                           "Immediate %1 out of valid range; must be within [%2 "
                           ": %3]")
                    .arg(field)
                    .arg(m_lowerBound)
                    .arg(m_upperBound);
            }
        }
        case Type::Register: {
            if (!ABInames.contains(field) &&
                !registerRegex.match(field, 0, QRegularExpression::NormalMatch).hasMatch()) {
                return QString("Register %1 is unrecognized").arg(field);
            } else {
                return QString();
            }
        }
        case Type::Offset: {
            return QString();
        }
    }
    return QString("Validation error");
}

AsmHighlighter::AsmHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
    createSyntaxRules();
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    HighlightingRule rule;

    // Create rules for name-specific registers
    regFormat.setForeground(QColor(Colors::FoundersRock));
    QStringList registerPatterns;
    registerPatterns << "\\bzero\\b"
                     << "\\bra\\b"
                     << "\\bsp\\b"
                     << "\\bgp\\b"
                     << "\\btp\\b"
                     << "\\bfp\\b";
    for (const auto& pattern : registerPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = regFormat;
        m_highlightingRules.append(rule);
    }

    // Create match cases for instructions
    QStringList instructionPatterns;
    instructionPatterns << "\\bla\\b"
                        << "\\brd\\b"
                        << "\\blw\\b"
                        << "\\blh\\b"
                        << "\\blb\\b"
                        << "\\bsb\\b"
                        << "\\bsh\\b"
                        << "\\bsw\\b"
                        << "\\bnop\\b"
                        << "\\bli\\b"
                        << "\\bmv\\b"
                        << "\\bnot\\b"
                        << "\\bneg\\b"
                        << "\\bnegw\\b"
                        << "\\bsext.w\\b"
                        << "\\bseqz\\b"
                        << "\\bsnez\\b"
                        << "\\bsltz\\b"
                        << "\\bsgtz\\b"
                        << "\\bbegz\\b"
                        << "\\bbnez\\b"
                        << "\\bblez\\b"
                        << "\\bbgez\\b"
                        << "\\bbltz\\b"
                        << "\\bbgtz\\b"
                        << "\\bbgt\\b"
                        << "\\bble\\b"
                        << "\\bbgtu\\b"
                        << "\\bbleu\\b"
                        << "\\bj\\b"
                        << "\\bjal\\b"
                        << "\\bjr\\b"
                        << "\\bjalr\\b"
                        << "\\bret\\b"
                        << "\\bcall\\b"
                        << "\\btail\\b"
                        << "\\bfence\\b"
                        << "\\brdinstret\\b"
                        << "\\brdcycle\\b"
                        << "\\brdtime\\b"
                        << "\\bcsrr\\b"
                        << "\\bcsrw\\b"
                        << "\\bcsrs\\b"
                        << "\\bcsrc\\b"
                        << "\\bcsrwi\\b"
                        << "\\bcsrsi\\b"
                        << "\\bcsrci\\b"
                        << "\\bauipc\\b"
                        << "\\baddi\\b"
                        << "\\baddi\\b"
                        << "\\bxori\\b"
                        << "\\bsub\\b"
                        << "\\bsubw\\b"
                        << "\\baddiw\\b"
                        << "\\bsltiu\\b"
                        << "\\bsltu\\b"
                        << "\\bslt\\b"
                        << "\\bbeq\\b"
                        << "\\bbne\\b"
                        << "\\bbge\\b"
                        << "\\bblt\\b"
                        << "\\bbltu\\b"
                        << "\\bbgeu\\b"
                        << "\\bsrli\\b"
                        << "\\bslli\\b"
                        << "\\bor\\b"
                        << "\\badd\\b"
                        << "\\becall\\b";
    instrFormat.setForeground(QColor(Colors::BerkeleyBlue));
    for (const auto& pattern : instructionPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = instrFormat;
        m_highlightingRules.append(rule);
    }

    // Create rules for registers that can easily be regex matched
    // (saved, temporary and argument registers)
    rule.pattern = QRegularExpression("\\b[(a|s|t|x)][0-9]{1,2}");
    rule.format = regFormat;
    m_highlightingRules.append(rule);

    // Create immediate hightlighting rule
    immFormat.setForeground(QColor(Qt::darkGreen));
    rule.pattern = QRegularExpression("(?<![A-Za-z])[-+]?\\d+");
    rule.format = immFormat;
    m_highlightingRules.append(rule);

    // Create comment highlighting rule
    commentFormat.setForeground(QColor(Colors::Medalist));
    rule.pattern = QRegularExpression("[#]+.*");
    rule.format = commentFormat;
    m_highlightingRules.append(rule);
}

void AsmHighlighter::highlightBlock(const QString& text) {
    if (checkSyntax(text) != QString()) {
        setFormat(0, text.length(), errorFormat);
    } else {
        foreach (const HighlightingRule& rule, m_highlightingRules) {
            QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                QRegularExpressionMatch match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }
}

void AsmHighlighter::createSyntaxRules() {
    // Create syntax rules for all base- and pseudoinstructions
    auto rule = SyntaxRule();
    QList<FieldType> types = QList<FieldType>();
    QStringList names;

    // nop
    rule.instr = "nop";
    rule.fields = 1;
    m_syntaxRules.insert(rule.instr, rule);

    // ecall
    rule.instr = "ecall";
    rule.fields = 1;
    m_syntaxRules.insert(rule.instr, rule);

    // call
    types.clear();
    types << FieldType(Type::Offset);
    rule.instr = "call";
    rule.fields = 2;
    m_syntaxRules.insert(rule.instr, rule);

    // jr
    types.clear();
    types << FieldType(Type::Register);
    rule.instr = "jr";
    rule.fields = 2;
    m_syntaxRules.insert(rule.instr, rule);

    // j
    types.clear();
    types << FieldType(Type::Register);
    rule.instr = "j";
    rule.fields = 2;
    m_syntaxRules.insert(rule.instr, rule);

    // li
    types.clear();
    types << FieldType(Type::Register) << FieldType(Type::Immediate, -2147483648, 2147483647);
    rule.instr = "li";
    rule.fields = 3;
    rule.inputs = types;
    m_syntaxRules.insert(rule.instr, rule);

    // 2-register pseudoinstructions
    types.clear();
    names.clear();
    types << FieldType(Type::Register) << FieldType(Type::Register);
    names << "mv"
          << "not"
          << "neg"
          << "negw"
          << "sext"
          << "seqz"
          << "snez"
          << "sltz"
          << "sgtz";
    for (const auto& name : names) {
        rule.instr = name;
        rule.fields = 3;
        rule.inputs = types;
        m_syntaxRules.insert(name, rule);
    }

    // Branch instructions
    types.clear();
    names.clear();
    types << FieldType(Type::Register) << FieldType(Type::Register) << FieldType(Type::Offset);
    names << "beg"
          << "bne"
          << "blt"
          << "bge"
          << "bltu"
          << "bgeu";
    for (const auto& name : names) {
        rule.instr = name;
        rule.fields = 4;
        rule.inputs = types;
        m_syntaxRules.insert(name, rule);
    }

    // I type instructions
    types.clear();
    names.clear();
    types << FieldType(Type::Register) << FieldType(Type::Register) << FieldType(Type::Immediate, -2048, 2047);
    names << "addi"
          << "slti"
          << "sltiu"
          << "xori"
          << "ori"
          << "andi"
          << "slli"
          << "srli"
          << "srai";
    for (const auto& name : names) {
        rule.instr = name;
        rule.fields = 4;
        rule.inputs = types;
        m_syntaxRules.insert(name, rule);
    }

    // Load instructions
    types.clear();
    names.clear();
    types << FieldType(Type::Register) << FieldType(Type::Immediate, -2048, 2047) << FieldType(Type::Register);
    names << "lb"
          << "lh"
          << "lw"
          << "lbu"
          << "lhu";

    for (const auto& name : names) {
        rule.instr = name;
        rule.fields = 4;
        rule.inputs = types;
        m_syntaxRules.insert(name, rule);
    }

    // R type instructions
    types.clear();
    names.clear();
    types << FieldType(Type::Register) << FieldType(Type::Register) << FieldType(Type::Register);
    names << "add"
          << "sub"
          << "sll"
          << "slt"
          << "sltu"
          << "xor"
          << "srl"
          << "sra"
          << "or"
          << "and";
    for (const auto& name : names) {
        rule.instr = name;
        rule.fields = 4;
        rule.inputs = types;
        m_syntaxRules.insert(name, rule);
    }

    // S type instructions
    types.clear();
    names.clear();
    types << FieldType(Type::Register) << FieldType(Type::Immediate, -2048, 2047) << FieldType(Type::Register);
    names << "sb"
          << "sh"
          << "sw";
    for (const auto& name : names) {
        rule.instr = name;
        rule.fields = 4;
        rule.inputs = types;
        m_syntaxRules.insert(name, rule);
    }
}

QString AsmHighlighter::checkSyntax(const QString& input) {
    const static auto splitter = QRegularExpression("(\\ |\\,|\\t|\\(|\\))");
    auto fields = input.split(splitter);
    fields.removeAll("");

    // Check for labels
    if (fields.size() > 0 && fields[0].at(fields[0].length() - 1) == ':') {
        fields.removeFirst();
    }

    // Validate remaining fields
    if (fields.size() > 0) {
        auto rule = m_syntaxRules.find(fields[0]);
        if (rule != m_syntaxRules.end()) {
            // A rule for the instruction has been found
            if (fields.size() == (*rule).fields) {
                // fields size is correct, check each instruction
                int index = 1;
                for (const auto& input : (*rule).inputs) {
                    QString res = input.validateField(fields[index]);
                    if (res == QString()) {
                        // continue
                        index++;
                    } else {
                        return res;
                    }
                }
            } else {
                // Invalid number of arguments
                return QString("Invalid number of arguments");
            }
        } else {
            return QString("Unknown instruction");
        }
    }
    return QString();
}