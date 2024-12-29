#include <QApplication>
#include <QRandomGenerator>
#include <QVector>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QDebug>
#include <QFileDialog>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include "qcustomplot.h"
#include <QDialog>
#include <QTableWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QRegularExpression>

class customPlotData{
public:
    customPlotData() : plot_(nullptr) {}
    customPlotData(QCustomPlot* plot) : plot_(plot){}
    void setPlot(QCustomPlot* plot){
        plot_ = plot;
    }
    QCustomPlot* plot_;
};
customPlotData plotDataa;

QVector<QVector<double>> transpose(const QVector<QVector<double>>& data) {
    QVector<QVector<double>> transposed(data[0].size(), QVector<double>(data.size()));
    for (int i = 0; i < data.size(); ++i) {
        for (int j = 0; j < data[i].size(); ++j) {
            transposed[j][i] = data[i][j];
        }
    }
    return transposed;
}

// Function to sort each column in descending order
void sortColumns(QVector<QVector<double>>& allYData) {
    // Check if allYData is not empty
    if (allYData.isEmpty()) return;

    // Transpose the data
    QVector<QVector<double>> transposed = transpose(allYData);

    // Sort each column (originally each row in transposed)
    for (auto& column : transposed) {
        std::sort(column.begin(), column.end());
    }

    // Transpose back to original format
    allYData = transpose(transposed);
}

class ParetoData {
public:
    ParetoData(const std::string &filename) {
        readDataFromFile(filename);
    }

    const QVector<QVector<double>>& getObjectiveData() const {
        return objectiveData;
    }

    const QVector<QVector<double>>& getVariableData() const {
        return variableData;
    }

    QVector<int> calculateParetoFront() const {
        QVector<int> front;
        for (int i = 0; i < objectiveData.size(); ++i) {
            bool isPareto = true;
            for (int j = 0; j < objectiveData.size(); ++j) {
                if (i == j) continue;
                bool dominates = true;
                for (int k = 0; k < objectiveData[i].size(); ++k) {
                    if (objectiveData[j][k] < objectiveData[i][k]) {
                        dominates = false;
                        break;
                    }
                }
                if (dominates) {
                    isPareto = false;
                    break;
                }
            }
            if (isPareto) {
                front.append(i);
            }
        }
        return front;
    }

    QPair<QVector<QVector<double>>, QVector<QVector<double>>> normalizeData() const {
        QVector<QVector<double>> normalizedPointData(variableData);
        QVector<QVector<double>> normalizedObjectiveData(objectiveData);

        // Normalize point data
        for (int j = 0; j < variableData[0].size(); ++j) {
            double minVal = variableData[0][j];
            double maxVal = variableData[0][j];
            for (int i = 1; i < variableData.size(); ++i) {
                minVal = qMin(minVal, variableData[i][j]);
                maxVal = qMax(maxVal, variableData[i][j]);
            }
            double range = maxVal - minVal;
            for (int i = 0; i < variableData.size(); ++i) {
                normalizedPointData[i][j] = range == 0 ? 0 : (variableData[i][j] - minVal) / range;
            }
        }

        // Normalize objective data
        for (int j = 0; j < objectiveData[0].size(); ++j) {
            double minVal = objectiveData[0][j];
            double maxVal = objectiveData[0][j];
            for (int i = 1; i < objectiveData.size(); ++i) {
                minVal = qMin(minVal, objectiveData[i][j]);
                maxVal = qMax(maxVal, objectiveData[i][j]);
            }
            double range = maxVal - minVal;
            for (int i = 0; i < objectiveData.size(); ++i) {
                normalizedObjectiveData[i][j] = range == 0 ? 0 : (objectiveData[i][j] - minVal) / range;
            }
        }

        return qMakePair(normalizedPointData, normalizedObjectiveData);
    }

private:
    QVector<QVector<double>> objectiveData;
    QVector<QVector<double>> variableData;

    void readDataFromFile(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cout << "Error opening file for reading. Check the file path and permissions.";
            return;
        }

        std::cout << "File opened successfully." << std::endl;

        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
                                             std::istream_iterator<std::string>());

            if (results.size() < 20) {
                std::cout << "Line " << lineCount << " has fewer than 20 elements." << std::endl;
                continue;
            }

            QVector<double> objectives;
            for (int j = 4; j < 10; ++j) {
                try {
                    objectives.push_back(std::stod(results[j]));
                } catch (const std::invalid_argument& e) {
                    std::cout << "Error converting objective value to double at line " << lineCount << ", column " << j << std::endl;
                    objectives.push_back(0.0); // or handle the error as needed
                } catch (const std::out_of_range& e) {
                    std::cout << "Objective value out of range at line " << lineCount << ", column " << j << std::endl;
                    objectives.push_back(0.0); // or handle the error as needed
                }
            }
            objectiveData.push_back(objectives);

            QVector<double> variables;
            for (int j = 10; j < results.size(); ++j) {
                try {
                    variables.push_back(std::stod(results[j]));
                } catch (const std::invalid_argument& e) {
                    std::cout << "Error converting attribute value to double at line " << lineCount << ", column " << j << std::endl;
                    variables.push_back(0.0); // or handle the error as needed
                } catch (const std::out_of_range& e) {
                    std::cout << "Attribute value out of range at line " << lineCount << ", column " << j << std::endl;
                    variables.push_back(0.0); // or handle the error as needed
                }
            }
            variableData.push_back(variables);


            ++lineCount;
        }

        std::cout << "Read " << lineCount << " lines from file." << std::endl;

        file.close();
    }
};

void plotDataObjectiveValueMethod(QCustomPlot* customPlot, const QVector<QVector<double>>& normalizedPointData, const QVector<QVector<double>>& normalizedObjectiveData, int numEntries, int numObjectives, const QVector<int>& front, QCPRange& initialXRange, QCPRange& initialYRange, int min, int max, int minObj,int maxObj) {
    customPlot->clearGraphs();
    customPlot->clearItems(); // Clear any existing items
    qDebug() << minObj;
    // Plot each solution as a separate graph with objectives as the y-axis and points as the x-axis
    for (int sol = 0; sol < numEntries; ++sol) {
        QVector<double> xData, yData;
        for (int obj = 0; obj < numObjectives; ++obj) {
            xData << obj; // Objectives on the x-axis
            yData << normalizedObjectiveData[sol][obj]; // Points on the y-axis
        }
        QVector<double> xDataBlue1, yDataBlue1, xDataRed, yDataRed, xDataBlue2, yDataBlue2;

        for (int obj = 0; obj < numObjectives; ++obj) {
            if (obj >= 0 && obj < minObj) {
                xDataBlue1 << xData[obj];
                yDataBlue1 << yData[obj];
            }
            else if(obj == minObj){
                xDataRed << xData[obj];
                yDataRed << yData[obj];
                xDataBlue1 << xData[obj];
                yDataBlue1 << yData[obj];
            }
            else if(obj>=minObj+1 && obj < maxObj){
                xDataRed << xData[obj];
                yDataRed << yData[obj];
            }
            else if(obj==maxObj){
                xDataRed << xData[obj];
                yDataRed << yData[obj];
                xDataBlue2 << xData[obj];
                yDataBlue2 << yData[obj];
            }
            else if (obj >= maxObj+1 && obj <= 5) {
                xDataBlue2 << xData[obj];
                yDataBlue2 << yData[obj];
            }
        }
        if (sol >= min && sol < max) {
            if (!xDataRed.isEmpty()) {
                QCPGraph *redGraph = customPlot->addGraph();
                redGraph->setData(xDataRed, yDataRed);
                QPen redPen(Qt::red);
                redPen.setWidth(1); // Set line width to normal
                redGraph->setPen(redPen);
                redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            // Plot the blue segment (6 to 8)
            if (!xDataBlue1.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue1, yDataBlue1);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            if (!xDataBlue2.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue2, yDataBlue2);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }
        } else {
            if (!xDataRed.isEmpty()) {
                QCPGraph *redGraph = customPlot->addGraph();
                redGraph->setData(xDataRed, yDataRed);
                QPen redPen(Qt::blue);
                redPen.setWidth(1); // Set line width to normal
                redGraph->setPen(redPen);
                redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            // Plot the blue segment (6 to 8)
            if (!xDataBlue1.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue1, yDataBlue1);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            if (!xDataBlue2.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue2, yDataBlue2);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }
        }

    }

    customPlot->yAxis->setTickLabelRotation(60);
    customPlot->yAxis->setSubTicks(false);
    customPlot->yAxis->setTickLength(0);
    customPlot->yAxis->grid()->setVisible(true);
    customPlot->yAxis->setRange(-0.03, 1.1); // Set y-axis range to [0, 1]

    // Customize the color and width of the x-axis grid lines
    QPen gridPen;
    gridPen.setColor(Qt::blue); // Set the color to blue
    gridPen.setStyle(Qt::SolidLine);
    gridPen.setWidth(2); // Increase line width for the x-axis grid lines
    customPlot->xAxis->grid()->setPen(gridPen);

    customPlot->xAxis->setRange(0, (numObjectives - 1) + 0.1); // Set x-axis range to [0, numObjectives - 1]
    QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
    ticker->setTickCount(30); // Set the desired number of ticks
    customPlot->yAxis->setTicker(ticker);
    customPlot->replot();

    // Store the initial ranges
    initialXRange = customPlot->xAxis->range();
    initialYRange = customPlot->yAxis->range();
    return;
}

void plotDataNormObjectiveMethod(QCustomPlot *customPlot, const QVector<QVector<double>> &normalizedPointData, const QVector<QVector<double>> &normalizedObjectiveData, int numEntries, int numObjectives, const QVector<int> &front, QCPRange &initialXRange, QCPRange &initialYRange,double min, double max, int minObj,int maxObj) {
    customPlot->clearGraphs();
    qDebug() << min << max << minObj << maxObj;
    customPlot->clearItems(); // Clear any existing items
    // Plot each solution as a separate graph with objectives as the y-axis and points as the x-axis
    for (int sol = 0; sol < numEntries; ++sol) {
        QVector<double> xData, yData;
        for (int obj = 0; obj < numObjectives; ++obj) {
            xData << obj; // Objectives on the x-axis
            yData << normalizedObjectiveData[sol][obj]; // Points on the y-axis
        }
        QVector<double> xData1, yData1, xData2, yData2,xDataB1, yDataB1, xDataB2, yDataB2, xData3, yData3,xDataB3, yDataB3, xData4, yData4, xData5, yData5, xData6, yData6,xDataB4, yDataB4,xDataB5, yDataB5,xDataB6, yDataB6;

        //if (obj == 0) {
        //if(0 <= obj && obj <= 5 && yData[obj] >= 0.1 && yData[obj] <= 0.5){

        for (int obj = 0; obj < numObjectives; ++obj) {
            if (obj == 0) {
                if(yData[obj]>=min && yData[obj]<=max && obj>=minObj && obj < maxObj){
                    xData1 << xData[obj];
                    yData1 << yData[obj];
                }
                else{
                    xDataB1 << xData[obj];
                    yDataB1 << yData[obj];
                }
            }
            else if(obj == 1){
                if(yData[obj]>=min && yData[obj]<=max && obj>=minObj && obj < maxObj){
                    xData2 << xData[obj];
                    yData2 << yData[obj];
                    xData1 << xData[obj];
                    yData1 << yData[obj];
                    xDataB1 << xData[obj];
                    yDataB1 << yData[obj];
                }
                else{
                    xDataB2 << xData[obj];
                    yDataB2 << yData[obj];
                    xDataB1 << xData[obj];
                    yDataB1 << yData[obj];
                    xData1 << xData[obj];
                    yData1 << yData[obj];
                }
            }
            else if(obj == 2){
                if(yData[obj]>=min && yData[obj]<=max && obj>=minObj && obj < maxObj){
                    xData3 << xData[obj];
                    yData3 << yData[obj];
                    xData2 << xData[obj];
                    yData2 << yData[obj];
                    xDataB2 << xData[obj];
                    yDataB2 << yData[obj];
                }
                else{
                    xDataB3 << xData[obj];
                    yDataB3 << yData[obj];
                    xData2 << xData[obj];
                    yData2 << yData[obj];
                    xDataB2 << xData[obj];
                    yDataB2 << yData[obj];
                }


            }
            else if(obj == 3){
                if(yData[obj]>=min && yData[obj]<=max && obj>=minObj && obj < maxObj){
                    xData4 << xData[obj];
                    yData4 << yData[obj];
                    xData3 << xData[obj];
                    yData3 << yData[obj];
                    xDataB3 << xData[obj];
                    yDataB3 << yData[obj];
                }
                else{
                    xDataB4 << xData[obj];
                    yDataB4 << yData[obj];
                    xData3 << xData[obj];
                    yData3 << yData[obj];
                    xDataB3 << xData[obj];
                    yDataB3 << yData[obj];
                }

            }
            else if (obj == 4) {
                if(yData[obj]>=min && yData[obj]<=max && obj>=minObj && obj < maxObj){
                    xData5 << xData[obj];
                    yData5 << yData[obj];
                    xData4 << xData[obj];
                    yData4 << yData[obj];
                    xDataB4 << xData[obj];
                    yDataB4 << yData[obj];
                }
                else{
                    xDataB5 << xData[obj];
                    yDataB5 << yData[obj];
                    xData4 << xData[obj];
                    yData4 << yData[obj];
                    xDataB4 << xData[obj];
                    yDataB4 << yData[obj];
                }
            }
            else if (obj == 5) {
                if(yData[obj]>=min && yData[obj]<=max && obj>=minObj && obj < maxObj){
                    xData6 << xData[obj];
                    yData6 << yData[obj];
                    xData5 << xData[obj];
                    yData5 << yData[obj];
                    xDataB5 << xData[obj];
                    yDataB5 << yData[obj];
                }
                else{
                    xDataB6 << xData[obj];
                    yDataB6 << yData[obj];
                    xData5 << xData[obj];
                    yData5 << yData[obj];
                    xDataB5 << xData[obj];
                    yDataB5 << yData[obj];
                }
            }
        }
        if (!xData1.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xData1, yData1);
            QPen redPen(Qt::red);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xData2.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xData2, yData2);
            QPen redPen(Qt::red);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xData3.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xData3, yData3);
            QPen redPen(Qt::red);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xData4.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xData4, yData4);
            QPen redPen(Qt::red);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xData5.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xData5, yData5);
            QPen redPen(Qt::red);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xData6.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xData6, yData6);
            QPen redPen(Qt::red);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }

        if (!xDataB1.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xDataB1, yDataB1);
            QPen redPen(Qt::blue);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xDataB2.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xDataB2, yDataB2);
            QPen redPen(Qt::blue);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xDataB3.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xDataB3, yDataB3);
            QPen redPen(Qt::blue);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xDataB4.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xDataB4, yDataB4);
            QPen redPen(Qt::blue);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xDataB5.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xDataB5, yDataB5);
            QPen redPen(Qt::blue);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }
        if (!xDataB6.isEmpty()) {
            QCPGraph *redGraph = customPlot->addGraph();
            redGraph->setData(xDataB6, yDataB6);
            QPen redPen(Qt::blue);
            redPen.setWidth(1); // Set line width to normal
            redGraph->setPen(redPen);
            redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
        }

    }

    customPlot->yAxis->setTickLabelRotation(60);
    customPlot->yAxis->setSubTicks(false);
    customPlot->yAxis->setTickLength(0);
    customPlot->yAxis->grid()->setVisible(true);
    customPlot->yAxis->setRange(-0.03, 1.1); // Set y-axis range to [0, 1]

    // Customize the color and width of the x-axis grid lines
    QPen gridPen;
    gridPen.setColor(Qt::blue); // Set the color to blue
    gridPen.setStyle(Qt::SolidLine);
    gridPen.setWidth(2); // Increase line width for the x-axis grid lines
    customPlot->xAxis->grid()->setPen(gridPen);

    customPlot->xAxis->setRange(0, (numObjectives - 1) + 0.1); // Set x-axis range to [0, numObjectives - 1]
    QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
    ticker->setTickCount(30); // Set the desired number of ticks
    customPlot->yAxis->setTicker(ticker);
    customPlot->replot();

    // Store the initial ranges
    initialXRange = customPlot->xAxis->range();
    initialYRange = customPlot->yAxis->range();
}

void plotDataRelativePositionMethod(QCustomPlot* customPlot, const QVector<QVector<double>>& normalizedPointData, const QVector<QVector<double>>& normalizedObjectiveData, int numEntries, int numObjectives, const QVector<int>& front, QCPRange& initialXRange, QCPRange& initialYRange, double min, double max, int minObj,int maxObj) {
    customPlot->clearGraphs();
    customPlot->clearItems(); // Clear any existing items
    min*=1925;
    max*=1925;
    QVector<QVector<double>> allYData;
    QVector<QVector<double>> allXData;
    // Plot each solution as a separate graph with objectives as the y-axis and points as the x-axis
    for (int sol = 0; sol < numEntries; ++sol) {

        QVector<double> xData, yData;
        for (int obj = 0; obj < numObjectives; ++obj) {
            xData << obj; // Objectives on the x-axis
            yData << normalizedObjectiveData[sol][obj]; // Points on the y-axis
        }

        allYData.append(yData);
        allXData.append(xData);
    }
    sortColumns(allYData);

    for (int sol = 0; sol < numEntries; ++sol){
        QCPGraph* graph = customPlot->addGraph();
        graph->setData(allXData[sol], allYData[sol]); // Swap x and y to plot objectives on x-axis and points on y-axis
        qDebug() << allYData[sol];
    }

    for (int sol = 0; sol < numEntries; ++sol) {
        QVector<double> xData, yData;
        for (int obj = 0; obj < numObjectives; ++obj) {
            xData << allXData[sol][obj]; // Objectives on the x-axis
            yData << allYData[sol][obj]; // Points on the y-axis
        }
        QVector<double> xDataBlue1, yDataBlue1, xDataRed, yDataRed, xDataBlue2, yDataBlue2;

        for (int obj = 0; obj < numObjectives; ++obj) {
            if (obj >= 0 && obj < minObj) {
                xDataBlue1 << xData[obj];
                yDataBlue1 << yData[obj];
            }
            else if(obj == minObj){
                xDataRed << xData[obj];
                yDataRed << yData[obj];
                xDataBlue1 << xData[obj];
                yDataBlue1 << yData[obj];
            }
            else if(obj>=minObj+1 && obj < maxObj){
                xDataRed << xData[obj];
                yDataRed << yData[obj];
            }
            else if(obj==maxObj){
                xDataRed << xData[obj];
                yDataRed << yData[obj];
                xDataBlue2 << xData[obj];
                yDataBlue2 << yData[obj];
            }
            else if (obj >= maxObj+1 && obj <= 5) {
                xDataBlue2 << xData[obj];
                yDataBlue2 << yData[obj];
            }
        }
        if (sol >= min && sol < max) {
            if (!xDataRed.isEmpty()) {
                QCPGraph *redGraph = customPlot->addGraph();
                redGraph->setData(xDataRed, yDataRed);
                QPen redPen(Qt::red);
                redPen.setWidth(1); // Set line width to normal
                redGraph->setPen(redPen);
                redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            // Plot the blue segment (6 to 8)
            if (!xDataBlue1.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue1, yDataBlue1);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            if (!xDataBlue2.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue2, yDataBlue2);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }
        } else {
            if (!xDataRed.isEmpty()) {
                QCPGraph *redGraph = customPlot->addGraph();
                redGraph->setData(xDataRed, yDataRed);
                QPen redPen(Qt::blue);
                redPen.setWidth(1); // Set line width to normal
                redGraph->setPen(redPen);
                redGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            // Plot the blue segment (6 to 8)
            if (!xDataBlue1.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue1, yDataBlue1);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }

            if (!xDataBlue2.isEmpty()) {
                QCPGraph *blueGraph = customPlot->addGraph();
                blueGraph->setData(xDataBlue2, yDataBlue2);
                QPen bluePen(Qt::blue);
                bluePen.setWidth(1); // Set line width to normal
                blueGraph->setPen(bluePen);
                blueGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 1));
            }
        }

    }

    customPlot->yAxis->setTickLabelRotation(60);
    customPlot->yAxis->setSubTicks(false);
    customPlot->yAxis->setTickLength(0);
    customPlot->yAxis->grid()->setVisible(true);
    customPlot->yAxis->setRange(-0.03, 1.1); // Set y-axis range to [0, 1]

    // Customize the color and width of the x-axis grid lines
    QPen gridPen;
    gridPen.setColor(Qt::blue); // Set the color to blue
    gridPen.setStyle(Qt::SolidLine);
    gridPen.setWidth(2); // Increase line width for the x-axis grid lines
    customPlot->xAxis->grid()->setPen(gridPen);

    customPlot->xAxis->setRange(0, (numObjectives - 1) + 0.1); // Set x-axis range to [0, numObjectives - 1]
    QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
    ticker->setTickCount(30); // Set the desired number of ticks
    customPlot->yAxis->setTicker(ticker);
    customPlot->replot();

    // Store the initial ranges
    initialXRange = customPlot->xAxis->range();
    initialYRange = customPlot->yAxis->range();
}

void plotData(QCustomPlot *customPlot, const QVector<QVector<double>> &normalizedPointData, const QVector<QVector<double>> &normalizedObjectiveData, int numEntries, int numObjectives, const QVector<int> &front, QCPRange &initialXRange, QCPRange &initialYRange) {
    customPlot->clearGraphs();
    customPlot->clearItems(); // Clear any existing items

    // Plot each solution as a separate graph with objectives as the y-axis and points as the x-axis
    for (int sol = 0; sol < numEntries; ++sol) {
        QCPGraph* graph = customPlot->addGraph();
        QVector<double> xData, yData;
        for (int obj = 0; obj < numObjectives; ++obj) {
            xData << obj; // Objectives on the x-axis
            yData << normalizedObjectiveData[sol][obj]; // Points on the y-axis
        }
        graph->setData(xData, yData); // Swap x and y to plot objectives on x-axis and points on y-axis
        QPen pen;
        pen.setStyle(Qt::SolidLine); // Change to a solid line
        pen.setWidth(1); // Set line width to normal for the graph lines
        pen.setColor(Qt::blue); // Change color to blue for Pareto-optimal solutions

        graph->setPen(pen);
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssSquare, 10));
    }

    customPlot->yAxis->setTickLabelRotation(60);
    customPlot->yAxis->setSubTicks(false);
    customPlot->yAxis->setTickLength(0);
    customPlot->yAxis->grid()->setVisible(true);
    customPlot->yAxis->setRange(-0.03, 1.1); // Set y-axis range to [0, 1]

    // Customize the color and width of the x-axis grid lines
    QPen gridPen;
    gridPen.setColor(Qt::blue); // Set the color to blue
    gridPen.setStyle(Qt::SolidLine);
    gridPen.setWidth(2); // Increase line width for the x-axis grid lines
    customPlot->xAxis->grid()->setPen(gridPen);

    customPlot->xAxis->setRange(0, (numObjectives - 1) + 0.1); // Set x-axis range to [0, numObjectives - 1]
    QSharedPointer<QCPAxisTicker> ticker(new QCPAxisTicker);
    ticker->setTickCount(30); // Set the desired number of ticks
    customPlot->yAxis->setTicker(ticker);
    customPlot->replot();

    // Store the initial ranges
    initialXRange = customPlot->xAxis->range();
    initialYRange = customPlot->yAxis->range();
}

void PlottingOnClick(QCustomPlot &customPlot, int i, double min, double max, int minObj,int maxObj){
    QCPRange initialXRange, initialYRange;

    // Declare a variable to store the filename
    QString filename;

    // Declare objectiveData in the outer scope
    QVector<QVector<double>> objectiveData;
    filename = "C:/Users/Lenovo/Documents/untitled1/stagetwofilterfile.txt";
    if (filename.isEmpty()) {
        return; // User cancelled the file dialog
    }

    std::cout << "Selected file: " << filename.toStdString() << std::endl;

    // Create a ParetoData instance and read the data from the file
    ParetoData paretoData(filename.toStdString());

    // Get the data
    objectiveData = paretoData.getObjectiveData();
    const QVector<QVector<double>>& variableData = paretoData.getVariableData();

    if (objectiveData.isEmpty() || variableData.isEmpty()) {
        qDebug() << "No data read from file.";
        return;
    }

    std::cout << "Read " << objectiveData.size() << " entries from file." << std::endl;

    // Calculate Pareto front
    QVector<int> front = paretoData.calculateParetoFront();

    // Normalize the data
    QPair<QVector<QVector<double>>, QVector<QVector<double>>> normalizedData = paretoData.normalizeData();

    // Plot the data
    if(i==1)
        plotData(&customPlot, normalizedData.first, normalizedData.second, normalizedData.first.size(), normalizedData.second[0].size(), front, initialXRange, initialYRange);
    else if(i==2){
        plotDataObjectiveValueMethod(&customPlot, normalizedData.first, normalizedData.second, normalizedData.first.size(), normalizedData.second[0].size(), front, initialXRange, initialYRange,min,max,minObj,maxObj);
    }
    else if(i==3){
        plotDataNormObjectiveMethod(&customPlot, normalizedData.first, normalizedData.second, normalizedData.first.size(), normalizedData.second[0].size(), front, initialXRange, initialYRange,min,max,minObj,maxObj);
    }
    else if(i==4){
        plotDataRelativePositionMethod(&customPlot, normalizedData.first, normalizedData.second, normalizedData.first.size(), normalizedData.second[0].size(), front, initialXRange, initialYRange,min,max,minObj,maxObj);
    }
    return;
}

class ObjectiveDialog : public QDialog {
public:
    ObjectiveDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setFixedSize(580, 260); // Set the size of the dialog

        QVBoxLayout *layout = new QVBoxLayout(this);

        tableWidget = new QTableWidget(6, 5, this);
        tableWidget->setHorizontalHeaderLabels(QStringList() << "Objectives" << "Enabled" << "Method" << "Min" << "Max");
        for (int i = 0; i < 6; ++i) {
            // Changed arg(i + 1) to arg(i) to start numbering from 0
            tableWidget->setItem(i, 0, new QTableWidgetItem(QString("Objective %1").arg(i)));
            tableWidget->setCellWidget(i, 1, new QCheckBox());
            if(i==2){
                QComboBox *comboBox = new QComboBox();
                comboBox->addItem("Objective Value");
                comboBox->addItem("Norm Objective");
                comboBox->addItem("Relative Position");
                connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, i](int index) {
                    QDoubleSpinBox *minSpinBox = static_cast<QDoubleSpinBox*>(tableWidget->cellWidget(i, 3));
                    QDoubleSpinBox *maxSpinBox = static_cast<QDoubleSpinBox*>(tableWidget->cellWidget(i, 4));
                    if (index == 0) { // Objective Value
                        minSpinBox->setRange(0, 1925); // Update the range to 0 to 1924
                        maxSpinBox->setRange(0, 1925); // Update the range to 0 to 1924
                    } else { // Norm Objective or Relative Position
                        minSpinBox->setRange(0, 1);
                        maxSpinBox->setRange(0, 1);
                    }
                });
                tableWidget->setCellWidget(i, 2, comboBox);

                QDoubleSpinBox *minSpinBox = new QDoubleSpinBox();
                minSpinBox->setRange(0, 1924); // Update the range to 0 to 1924
                tableWidget->setCellWidget(i, 3, minSpinBox);

                QDoubleSpinBox *maxSpinBox = new QDoubleSpinBox();
                maxSpinBox->setRange(0, 1924); // Update the range to 0 to 1924
                tableWidget->setCellWidget(i, 4, maxSpinBox);
            }
        }

        layout->addWidget(tableWidget);

        okButton = new QPushButton("OK", this);
        layout->addWidget(okButton);
        QObject::connect(okButton, &QPushButton::clicked, [this]() {
            QDialog *newWindow = new QDialog(this);
            newWindow->setWindowTitle("New Window");
            // Create layout and widgets
            QVBoxLayout *layout = new QVBoxLayout(newWindow);
            QCustomPlot *customPlot = new QCustomPlot();
            customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // Enable user interactions for zooming and panning
            layout->addWidget(customPlot);
            // Set the layout for the dialog
            newWindow->setLayout(layout);
            // Show the dialog
            newWindow->showMaximized();
            QDoubleSpinBox *minSpinBox = static_cast<QDoubleSpinBox*>(tableWidget->cellWidget(2, 3));
            QDoubleSpinBox *maxSpinBox = static_cast<QDoubleSpinBox*>(tableWidget->cellWidget(2, 4));
            double min = minSpinBox->value();
            double max = maxSpinBox->value();
            int minObj =0;
            int maxObj =0;
            bool firstFound = false;
            for (int i = 0; i < 6; ++i) {
                QCheckBox *checkBox = qobject_cast<QCheckBox*>(tableWidget->cellWidget(i, 1));
                if(!firstFound){
                    if (checkBox && checkBox->isChecked()) {
                        minObj = i;
                        firstFound = true;
                    }
                }
                if(firstFound){
                    if (checkBox && checkBox->isChecked()) {
                        maxObj = i;
                    }
                }
            }
            QComboBox *comboBox = qobject_cast<QComboBox*>(tableWidget->cellWidget(2, 2));
            QString text = comboBox->currentText();
            if(text.toStdString() == "Objective Value")
                PlottingOnClick(*customPlot,2,min,max,minObj,maxObj);
            else if(text.toStdString() == "Norm Objective")
                PlottingOnClick(*customPlot,3,min,max,minObj,maxObj);
            else if(text.toStdString() == "Relative Position")
                PlottingOnClick(*customPlot,4,min,max,minObj,maxObj);
            return;
        });
    }


    QTableWidget *tableWidget;
    QPushButton *okButton;
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // Create a widget to hold the plot and the button
    QWidget window;
    QVBoxLayout *layout = new QVBoxLayout(&window);

    // Create QCustomPlot widget
    QCustomPlot *customPlot = new QCustomPlot();
    plotDataa.setPlot(customPlot);
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom); // Enable user interactions for zooming and panning
    layout->addWidget(customPlot);

    // Create an equalizer button
    QPushButton *equalizerButton = new QPushButton("Filtering");
    layout->addWidget(equalizerButton);

    // Create a button
    QPushButton *button = new QPushButton("Open Pareto Data File");
    layout->addWidget(button);

    // Create a button for resetting the zoom level
    QPushButton *resetZoomButton = new QPushButton("Reset Zoom");
    layout->addWidget(resetZoomButton);

    // Declare variables to store the initial ranges
    QCPRange initialXRange, initialYRange;

    // Declare a variable to store the filename
    QString filename;

    // Declare objectiveData in the outer scope
    QVector<QVector<double>> objectiveData;

    // Connect the button's clicked signal to a slot that changes the data
    QObject::connect(button, &QPushButton::clicked, [customPlot]() {
        PlottingOnClick(*customPlot, 1,0,0,0,0);
    });

    // Connect the equalizerButton's clicked signal to a slot that opens an ObjectiveDialog
    QObject::connect(equalizerButton, &QPushButton::clicked, [&]() {
        ObjectiveDialog dialog;
        if (dialog.exec() == QDialog::Accepted) {
            QVector<bool> visibleSolutions(objectiveData.size(), true); // Initialize visibility for all solutions
        }
    });

    // Connect the resetZoomButton's clicked signal to a lambda function that resets the ranges
    QObject::connect(resetZoomButton, &QPushButton::clicked, customPlot, [customPlot, &initialXRange, &initialYRange]() {
        customPlot->xAxis->setRange(initialXRange);
        customPlot->yAxis->setRange(initialYRange);
        customPlot->replot();
    });
    window.showMaximized();
    return a.exec();
}


