#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace std;
using namespace std::filesystem;

/// <summary>
/// Класс, представляющий метаданные проекта.
/// Содержит информацию о названии проекта.
/// </summary>
class ProjectMetadata
{
    public:
    /// <summary>
    /// Название проекта.
    /// </summary>
    /// <value>
    /// Строковое значение, представляющее название проекта.
    /// </value>
    string name;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectMetadata, name)
};

/// <summary>
/// Класс, представляющий метаданные, связанные с сериализацией графа и генерацией Verilog.
/// </summary>
class GraphVerilogMetadata
{
    public:
    /// <summary>
    /// Флаг, указывающий, был ли сериализован граф.
    /// </summary>
    /// <value>
    /// true, если граф был сериализован; в противном случае — false.
    /// </value>
    bool graphSerialized = false;
    /// <summary>
    /// Флаг, указывающий, был ли сгенерирован Verilog код.
    /// </summary>
    /// <value>
    /// true, если Verilog код был сгенерирован; в противном случае — false.
    /// </value>
    bool verilogGenerated = false;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(GraphVerilogMetadata, graphSerialized, verilogGenerated)
};

/// <summary>
/// Класс, представляющий метаданные, связанные с компиляцией в Quartus.
/// </summary>
class QuartusMetadata
    {
    public:
    /// <summary>
    /// Флаг, указывающий, была ли выполнена компиляция в Quartus.
    /// </summary>
    /// <value>
    /// true, если компиляция в Quartus была выполнена; в противном случае — false.
    /// </value>
    bool quartusCompiled = false;

    /// <summary>
    /// Название устройства, для которого выполнялась компиляция в Quartus.
    /// </summary>
    /// <value>
    /// Строковое значение, представляющее название устройства.
    /// </value>
     string deviceName = "5CGXFC9E7F35C8";
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(QuartusMetadata, quartusCompiled, deviceName)

};

/// <summary>
/// Класс, представляющий метаданные, связанные с базой данных.
/// </summary>
class DatabaseMetadata {
    public:
    /// <summary>
    /// IP-адрес базы данных.
    /// </summary>
    /// <value>
    /// Строковое значение, представляющее IP-адрес базы данных.
    /// </value>
    string dbIp;
    /// <summary>
    /// Имя пользователя базы данных.
    /// </summary>
    /// <value>
    /// Строковое значение, представляющее имя пользователя базы данных.
    /// </value>
    string dbUsername;
    /// <summary>
    /// Пароль для доступа к базе данных.
    /// </summary>
    /// <value>
    /// Массив байтов, представляющий зашифрованный пароль для доступа к базе данных.
    /// </value>
    string dbPassword;// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    /// <summary>
    /// Название базы данных.
    /// </summary>
    /// <value>
    /// Строковое значение, представляющее название базы данных.
    /// </value>
    string dbName;
    /// <summary>
    /// Порт базы данных.
    /// </summary>
    /// <value>
    /// Целочисленное значение, представляющее порт базы данных.
    /// </value>
    int dbPort = -1;
    /// <summary>
    /// Флаг, указывающий, были ли данные записаны в базу данных.
    /// </summary>
    /// <value>
    /// true, если данные были записаны в базу данных; в противном случае — false.
    /// </value>
    bool writtenToDB = false;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DatabaseMetadata, dbIp, dbUsername, dbPassword, dbName, dbPort, writtenToDB)
};

/// <summary>
/// Класс, представляющий настройки проекта.
/// Содержит все метаданные, относящиеся к проекту.
/// </summary>
class ProjectSettings {
public:
    /// <summary>
    /// Метаданные проекта.
    /// </summary>
    /// <value>
    /// Экземпляр класса ProjectMetadata, содержащий информацию о проекте.
    /// </value>
    ProjectMetadata projectMetadata;
    /// <summary>
    /// Метаданные, связанные с сериализацией графа и генерацией Verilog.
    /// </summary>
    /// <value>
    /// Экземпляр класса GraphVerilogMetadata.
    /// </value>
    GraphVerilogMetadata graphVerilogMetadata;
    /// <summary>
    /// Метаданные, связанные с компиляцией в Quartus.
    /// </summary>
    /// <value>
    /// Экземпляр класса QuartusMetadata.
    /// </value>
    QuartusMetadata quartusMetadata;
    /// <summary>
    /// Метаданные, связанные с базой данных.
    /// </summary>
    /// <value>
    /// Экземпляр класса DatabaseMetadata.
    /// </value>
    DatabaseMetadata databaseMetadata;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProjectSettings, projectMetadata, graphVerilogMetadata, quartusMetadata, databaseMetadata)
};

string readFile(const string& path) {
    ifstream file(path);
    if (!file.is_open())
        throw runtime_error("Не удалось открыть файл: " + path);
    return string((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

/// <summary>
/// Главная точка входа приложения.
/// Обрабатывает аргументы командной строки для выполнения различных действий, таких как открытие, создание, удаление или переименование проекта.
/// </summary>
/// <param name="argc">Целое число, содержащее количество аргументов, которые следуют в argv.</param>
/// <param name="argv">Массив завершающихся null строк, представляющих введенные пользователем программы аргументы командной строки.</param>
int main(int argc, char *argv[]) {
    // Инициализация переменных для хранения параметров командной строки.
    string location; // Расположение проекта.
    string name;     // Имя проекта.
    string action = "o";   // Действие, которое необходимо выполнить (o - открыть, c - создать, e - удалить, r - переименовать). По умолчанию "o".
    string new_name; // Новое имя проекта (используется при переименовании).
    // Итерация по аргументам командной строки.
    for (int i = 1; i < argc; i++) {
        string option = argv[i];
        // Обработка аргументов с помощью switch.
        if (option == "-l"||option == "--location") {
            if (i < argc - 1)
            {
                location = argv[++i];// Получение значения расположения проекта из следующего аргумента.
            }
            else
            {
                cout<<("No project location provided");
                exit(1);
            }
            
        }
        else if (option == "-n"||option == "--name") {
            if (i < argc - 1)
            {
                name = argv[++i]; // Получение значения имени проекта из следующего аргумента.
            }
            else
            {
                cout<<("No project name provided");
                exit(1);
            }
            
        }
        else if (option == "-o"||option ==  "--open"){
            action = "o"; // Установка действия "открыть".
            
        }
        else if (option == "-c"||option ==  "--create"){
            action = "c"; // Установка действия "создать".
            
        }
        else if (option == "-e"||option ==  "--erase"){
            action = "e"; // Установка действия "удалить".
            
        }
        else if (option == "-r"||option ==  "--rename"){
            action = "r"; // Установка действия "переименовать".
            if (i < argc - 1)
            {
                new_name = argv[++i]; // Получение нового имени проекта из следующего аргумента.
            }
            else
            {
                cout<<("No new name provided");
                exit(1);
            }
            
        }
        else {
            cout<<("Argument "+option+" is invalid"); // Вывод сообщения об ошибке.
            exit(1); // Завершение программы с кодом ошибки 1.
        }
    }
    // Формирование путей к файлам и папкам проекта на основе полученных параметров.
    string metadata_location      = location + "/" + name + "_metadata.json";
    string new_metadata_location  = location + "/" + new_name + "_metadata.json";
    string graph_location         = location + "/" + name + "_graph_object_serialized.json";
    string new_graph_location     = location + "/" + new_name + "_graph_object_serialized.json";
    string verilog_location       = location + "/" + name + "_NoC_description";
    string new_verilog_location   = location + "/" + new_name + "_NoC_description";
    // Обработка действий "открыть" и "переименовать".
    if(action == "o"|| action == "r") {
        // Проверка существования директории проекта.
        if (!exists(location))
        {
            cout<<("Failed to find project directory"); // Вывод сообщения об ошибке.
            exit(1);                                  // Завершение программы с кодом ошибки 1.
        }

        // Проверка существования файла метаданных проекта.
        if (!exists(metadata_location))
        {
            cout<<("Failed to find project metadata"); // Вывод сообщения об ошибке.
            exit(1);                                  // Завершение программы с кодом ошибки 1.
        }

        string json = "";
        try
        {
            // Чтение файла метаданных проекта.
            json = readFile(metadata_location);
        }
        catch (exception e)
        {
            cout<<"Failed to read project metadata: "<<e.what(); // Вывод сообщения об ошибке.
            exit(1); // Завершение программы с кодом ошибки 1.
        }
        ProjectSettings projectSettings;

        try
        {
            // Десериализация JSON-данных в объект ProjectSettings.
            projectSettings = json::parse(json).get<ProjectSettings>();
            // Проверка соответствия имени проекта в метаданных заданному имени.
            if (projectSettings.projectMetadata.name != name)
            {
                cout<<("Wrong project name in the metadata. Manual fixing of the .json file is needed"); // Вывод сообщения об ошибке.
                exit(1); // Завершение программы с кодом ошибки 1.
            }
        }
        catch (exception e)
        {
            cout<<"Failed to read project metadata: "<<e.what(); // Вывод сообщения об ошибке.
            exit(1); // Завершение программы с кодом ошибки 1.
        }


        // Если действие - "переименовать".
        if (action == "r")
        {
            // Изменение имени проекта в объекте ProjectSettings.
            projectSettings.projectMetadata.name = new_name;
            // Сериализация объекта ProjectSettings в JSON-формат с отступами.
            json = nlohmann::json(projectSettings).dump(4);

            try
            {
                ofstream(new_metadata_location) << json;

                // Удаление старого файла метаданных.
                remove(metadata_location);

                // Перемещение файла графа, если он существует.
                if (exists(graph_location))
                {
                    rename(graph_location, new_graph_location);
                }
                // Перемещение файла метаданных, если он существует.
                if (exists(metadata_location))
                {
                    rename(metadata_location, new_metadata_location);
                }
                // Перемещение директории Verilog-описания, если она существует.
                if (exists(verilog_location))
                {
                    rename(verilog_location, new_verilog_location);
                }
            }
            catch (exception e)
            {
                cout<<"Failed to rename the project: "<<e.what(); // Вывод сообщения об ошибке.
                exit(1); // Завершение программы с кодом ошибки 1.
            }
        }

    }

    // Обработка действия "создать".
    else if(action == "c") {
        // Проверка существования директории проекта.
        if (!exists(location))
        {
            try
            {
                // Создание директории проекта, если она не существует.
                create_directories(location);
            }
            catch (exception e)
            {
                cout<<"Failed to create a project directory: "<<e.what(); // Вывод сообщения об ошибке.
                exit(1); // Завершение программы с кодом ошибки 1.
            }
        }

        // Проверка существования файла метаданных проекта.
        if (exists(metadata_location))
        {
            cout<<("This project already exists"); // Вывод сообщения об ошибке.
            exit(1);                                  // Завершение программы с кодом ошибки 1.
        }

        // Создание объекта ProjectSettings и установка имени проекта.
        ProjectSettings project_settings;
        project_settings.projectMetadata.name = name;
        // Сериализация объекта ProjectSettings в JSON-формат с отступами.
        string serialized = nlohmann::json(project_settings).dump(4);

        try
        {
            // Создание файла метаданных проекта и запись сериализованных данных.
            ofstream(metadata_location) << serialized;
        }
        catch (exception e)
        {
            cout<<"Failed to create the project metadata: "<<e.what(); // Вывод сообщения об ошибке.
            exit(1); // Завершение программы с кодом ошибки 1.
        }

    }
    // Обработка действия "удалить".
    else if(action == "e") {
        // Проверка существования директории проекта.
        if (!exists(location))
        {
            cout<<("Non-existent directory"); // Вывод сообщения об ошибке.
            exit(0);                        // Завершение программы с кодом 0.
        }

        // Проверка существования файла метаданных проекта.
        if (!exists(metadata_location))
        {
            cout<<("Non-existent project");   // Вывод сообщения об ошибке.
            exit(0);                        // Завершение программы с кодом 0.
        }

        try
        {
            // Удаление файла графа, если он существует.
            if (exists(graph_location))
            {
                remove(graph_location);
            }
            // Удаление файла метаданных, если он существует.
            if (exists(metadata_location))
            {
              remove(metadata_location);
            }
            // Удаление директории Verilog-описания, если она существует.
            if (exists(verilog_location))
            {
                remove_all(verilog_location);
            }
        }
        catch (exception e)
        {
            cout<<"Failed to delete the project: "<<e.what(); // Вывод сообщения об ошибке.
            exit(1); // Завершение программы с кодом ошибки 1.
        }
    }
}