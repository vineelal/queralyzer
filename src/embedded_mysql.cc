#include "embedded_mysql.h"
extern int queralyzer_parser (const char *queryBuffer,
			      std::vector < std::string >
			      *createQueriesVector,
			      TableMetaData ** tableData,
			      IndexMetaData ** indexData);
bool
  EmbeddedMYSQL::isInstantiated = false;
EmbeddedMYSQL *
  EmbeddedMYSQL::my_embedded_mysql = NULL;
bool
  EmbeddedMYSQL::isInitialized = false;
MYSQL *
  EmbeddedMYSQL::mysql = NULL;

EmbeddedMYSQL *
EmbeddedMYSQL::getInstance ()
{
  if (isInstantiated)
    {
      return my_embedded_mysql;
    }
  else
    {
      my_embedded_mysql = new EmbeddedMYSQL ();
      isInstantiated = true;
      isInitialized = false;
      return my_embedded_mysql;
    }
}

int
EmbeddedMYSQL::initializeMYSQL ()
{
  if (mysql)
    return 0;
  /* Currently these are hard coded, implmentation may be changed in future */
  char *server_options[] = { "mysql_test",
    "--defaults-file=my.init",
    NULL
  };
  int num_elements = (sizeof (server_options) / sizeof (char *)) - 1;
  char *server_groups[] = { "server", "client", NULL };

  mysql_library_init (num_elements, server_options, server_groups);
  mysql = mysql_init (NULL);
  if (mysql)
    {
      mysql_options (mysql, MYSQL_READ_DEFAULT_GROUP, "libmysqld_client");
      mysql_options (mysql, MYSQL_OPT_USE_EMBEDDED_CONNECTION, NULL);

      if (!mysql_real_connect (mysql, NULL, NULL, NULL, "test", 0, NULL, 0))
	{
	  std::
	    cout << "mysql_real_connect failed: " << mysql_error (mysql) <<
	    std::endl;
	  isInitialized = false;
	  return -1;
	}

      if (mysql_query
	  (mysql,
	   "create table IF NOT EXISTS test_aps (id int primary key, name varchar(100))"))
	{
	  isInitialized = false;
	  return -1;
	}
      isInitialized = true;
      return 0;
    }
  else
    {
      std::cout << "mysql was never inited succesfully" << std::endl;
      isInitialized = false;
      return -1;
    }
}

int
EmbeddedMYSQL::deinitializeMYSQL ()
{
  mysql_close (mysql);
  mysql_library_end ();
  return 0;
}

int
EmbeddedMYSQL::executeMYSQL (std::string query_str)
{
  if (isInitialized == false)
    {
      std::
	cerr <<
	"MYSQL is not initialised properly, unable to process queries";
    }
  if (mysql_query (mysql, query_str.c_str ()))
    {
      std::cerr << "problems running " << query_str << " error " <<
	mysql_error (mysql);
      return 1;
    }
  else
    {
      //mysql_free_result(results);
      return 0;
    }

}

int
EmbeddedMYSQL::executeMYSQL (std::string query_str,
			     ExplainMetaData * explain_data)
{
  if (isInitialized == false)
    {
      std::
	cerr <<
	"MYSQL is not initialised properly, unable to process queries";
    }
  if (mysql_query (mysql, query_str.c_str ()))
    {
      std::cerr << "problems running " << query_str << " error " <<
	mysql_error (mysql);
      return 1;
    }
  else
    {
      if (mysql_field_count (mysql) > 0)
	{
	  results = mysql_store_result (mysql);

	  // used to determine if the query returned any results
	  if (results)
	    {
	      /* Store the values in explain_data */
	      int i = 0;
	      while (row = mysql_fetch_row (results))
		{
		  explain_data[i].id =
		    *(row + 0) ? (char *) *(row + 0) : "NULL";
		  explain_data[i].select_type =
		    *(row + 1) ? (char *) *(row + 1) : "NULL";
		  explain_data[i].table =
		    *(row + 2) ? (char *) *(row + 2) : "NULL";
		  explain_data[i].type =
		    *(row + 3) ? (char *) *(row + 3) : "NULL";
		  explain_data[i].possible_keys =
		    *(row + 4) ? (char *) *(row + 4) : "NULL";
		  explain_data[i].key =
		    *(row + 5) ? (char *) *(row + 5) : "NULL";
		  explain_data[i].key_len =
		    *(row + 6) ? (char *) *(row + 6) : "NULL";
		  explain_data[i].ref =
		    *(row + 7) ? (char *) *(row + 7) : "NULL";
		  explain_data[i].rows =
		    *(row + 8) ? (char *) *(row + 8) : "NULL";
		  explain_data[i].Extra =
		    *(row + 9) ? (char *) *(row + 9) : "NULL";
		  i++;
		}
	    }
	}
      mysql_free_result (results);
      return 0;
    }
}

void
EmbeddedMYSQL::getTableMetaDataMYSQL (std::string & table_json_output)
{
  JsonSerializer::serialize (table_data, table_json_output);
  return;
}

void
EmbeddedMYSQL::setTableMetaDataMYSQL (std::string & table_json_input)
{
  TableMetaData *temp_table_data = new TableMetaData[10];
  JsonSerializer::deserialize (temp_table_data, table_json_input);
  for (int i = 0; i < 10; i++)
    {
      if (!(temp_table_data[i]).tableName.empty ())
	{
	  for (int j = 0; j < 10; j++)
	    {
	      if ((temp_table_data[i]).tableName == table_data[j].tableName)
		{
		  //Update the table_data object here. (pathetic way of doing it)
		  table_data[j] = (temp_table_data[i]);
		}
	    }
	}
    }

  return;
}

void
EmbeddedMYSQL::getIndexMetaDataMYSQL (std::string & index_json_output)
{
  JsonSerializer::serialize (index_data, index_json_output);
  return;
}

void
EmbeddedMYSQL::setIndexMetaDataMYSQL (std::string & index_json_input)
{
  JsonSerializer::deserialize (index_data, index_json_input);
  return;
}

int
EmbeddedMYSQL::parseMYSQL (std::string query_str,
			   std::vector < std::string > *create_queries_vector)
{
  return (queralyzer_parser (query_str.c_str (), create_queries_vector,
			     &table_data, &index_data));
}