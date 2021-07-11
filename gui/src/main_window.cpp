#include "main_window.h"

#include <QFile>
#include <QTimer>
#include <QToolBar>
#include <QLineEdit>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>

#include "ptx_executor.h"
#include "ptx_generator.h"
#include "ptx_interpreter.h"

#include "code_editor.hpp"
#include "cuda_highlighter.hpp"
#include "ptx_highlighter.hpp"
#include "syntax_style.h"

MainWindow::MainWindow()
  : options(new QLineEdit())
  , cuda(new CodeEditor())
  , ptx(new CodeEditor())
  , timer(new QTimer())
{
  options->setText("-lineinfo, --gpu-architecture=compute_86");

  cuda->setAcceptRichText(false);
  cuda->setPlainText("\n"
                     "// Vector add example\n"
                     "extern \"C\" __global__ void kernel(int n, const int *x, const int *y, int *result)\n"
                     "{\n"
                     "  const unsigned int i = threadIdx.x + blockIdx.x * blockDim.x;\n"
                     "  \n"
                     "  if (i < n) \n"
                     "  {\n"
                     "    result[i] = x[i] + y[i];\n"
                     "  }\n"
                     "}\n");

  QHBoxLayout *h_layout = new QHBoxLayout();
  h_layout->addWidget(cuda);
  h_layout->addWidget(ptx);

  QVBoxLayout *v_layout = new QVBoxLayout();
  v_layout->addWidget(options);
  v_layout->addLayout(h_layout);

  chart = new QChart();
  chart->setBackgroundBrush(QBrush(QColor("#282a36")));

  chart_view = new QChartView(chart);
  chart_view->setBackgroundBrush(QBrush(QColor("#282a36")));
  chart_view->setRenderHint(QPainter::Antialiasing);
  chart_view->hide();

  line_series = new QLineSeries();
  scatter_series = new QScatterSeries();

  QPen pen = line_series->pen();
  pen.setWidth(3);
  pen.setBrush(QBrush("#50FA7B"));

  line_series->setPen(pen);

  scatter_series->setColor(QColor("#F8F8F2"));

  chart->addSeries(line_series);
  chart->addSeries(scatter_series);
  chart->createDefaultAxes();
  chart->legend()->hide();

  chart->axes(Qt::Vertical).back()->setLabelsColor(QColor("#F8F8F2"));
  chart->axes(Qt::Horizontal).back()->setLabelsColor(QColor("#F8F8F2"));

  v_layout->addWidget(chart_view);

  QWidget *central_widget = new QWidget();
  central_widget->setLayout(v_layout);

  setCentralWidget(central_widget);

  timer->setSingleShot(true);

  tool_bar = addToolBar("Interpreter");
  QWidget* spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  tool_bar->addWidget(spacer);

  run_action = new QAction(QIcon(":/icons/play.png"), "Run", this);
  interpret_action = new QAction(QIcon(":/icons/bug.png"), "Interpret", this);

  tool_bar->addAction(run_action);
  tool_bar->addAction(interpret_action);
  tool_bar->setMovable(false);

  QObject::connect(interpret_action, &QAction::triggered, this, &MainWindow::interpret);
  QObject::connect(run_action, &QAction::triggered, this, &MainWindow::execute);

  QObject::connect(cuda->document(), &QTextDocument::contentsChanged, this, &MainWindow::reset_timer);
  QObject::connect(options, &QLineEdit::textChanged, this, &MainWindow::reset_timer);
  QObject::connect(timer, &QTimer::timeout, this, &MainWindow::regen_ptx);

  QFont jet_brains_mono = QFont("JetBrains Mono", 12);
  cuda->setFont(jet_brains_mono);
  options->setFont(jet_brains_mono);

  cuda->setAutoIndentation(true);
  cuda->setAutoParentheses(true);
  cuda->setTabReplace(true);
  cuda->setTabReplaceSize(2);

  ptx->setFont(jet_brains_mono);
  ptx->setReadOnly(true);

  load_style(":/style/dracula.xml");

  cuda->setHighlighter(new CUDAHighlighter());
  ptx->setHighlighter(new PTXHighlighter());

  setStyleSheet("QToolBar {"
                " background-color: #414450;"
                "}"
                ""
                "QToolButton {"
                " max-width: 15px; "
                " max-height: 15px; "
                " margin: 8px 4px; "
                "}"
                ""
                "QToolButton:hover {"
                " background-color: #282a36; "
                "}"
                ""
                "QLineEdit {"
                " background-color: #282a36; "
                " color: #F8F8F2; "
                "}"
                ""
                "QScrollBar:vertical {"
                "    padding: 0px 0px 0px 0px;"
                "    background: #282a36;"
                "}"
                "QScrollBar::handle:vertical {"
                "    background: #4c4d56;"
                "}"
                ""
                "QMainWindow {"
                " background-color: #414450; "
                "}");

  reset_timer();
}

MainWindow::~MainWindow() = default;

void MainWindow::load_style(QString path)
{
    QFile fl(path);

    if (!fl.open(QIODevice::ReadOnly))
    {
        return;
    }

    auto style = new SyntaxStyle(this);

    if (!style->load(fl.readAll()))
    {
        delete style;
        return;
    }

    cuda->setSyntaxStyle(style);
    ptx->setSyntaxStyle(style);
}

void MainWindow::interpret()
{
  PTXInterpreter interpreter;

  interpreter.interpret(ptx->toPlainText().toStdString());
}

void MainWindow::reset_timer()
{
  timer->start(1000);

  run_action->setEnabled(false);
  interpret_action->setEnabled(false);
}

void MainWindow::regen_ptx()
{
  std::string cuda_source = cuda->toPlainText().toStdString();

  QStringList options_list = options->text().split(',');
  std::vector<std::string> str_options_list(options_list.size());
  std::vector<const char *> c_str_options_list(options_list.size());

  for (int i = 0; i < static_cast<int>(c_str_options_list.size()); i++)
  {
    str_options_list[i] = options_list[i].toStdString();
    c_str_options_list[i] = str_options_list[i].c_str();
  }

  PTXGenerator generator;
  std::optional<PTXCode> ptx_code = generator.gen(cuda_source.c_str(), c_str_options_list);

  if (ptx_code)
  {
    ptx->setPlainText(ptx_code->get_ptx());
  }
  else
  {
    ptx->setPlainText("Compilation error");
  }

  run_action->setEnabled(true);
  interpret_action->setEnabled(true);
}

void MainWindow::execute()
{
    execution_id++;

    if (!executor)
    {
        executor = std::make_unique<PTXExecutor>();
        chart_view->show();
    }

    std::string ptx_code = ptx->toPlainText().toStdString();

    // TODO Parameter manager
    void* kernel_args[4];

    float elapsed = executor->execute(
            kernel_args,
            256,
            256 * 1024,
            ptx_code.c_str());

    min_elapsed = std::min(min_elapsed, elapsed);
    max_elapsed = std::max(max_elapsed, elapsed);

    line_series->append(static_cast<double>(execution_id), elapsed);
    scatter_series->append(static_cast<double>(execution_id), elapsed);

    chart->axes(Qt::Horizontal).back()->setRange(0, execution_id + 1);
    chart->axes(Qt::Vertical).back()->setRange(min_elapsed - min_elapsed / 4, max_elapsed + max_elapsed / 4);
}
