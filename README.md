# SQLite database module for udjat

Allow storing url based alerts in a sqlite database to avoid missing then in case of a network or server failure.

[![License: GPL v3](https://img.shields.io/badge/License-GPL%20v3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
![CodeQL](https://github.com/PerryWerneck/udjat-module-sqlite/workflows/CodeQL/badge.svg?branch=master)
[![build result](https://build.opensuse.org/projects/home:PerryWerneck:udjat/packages/udjat-module-sqlite/badge.svg?type=percent)](https://build.opensuse.org/package/show/home:PerryWerneck:udjat/udjat-module-sqlite)

## Using agent

### Examples

[Udjat](../../../udjat) service configuration to store an [user module](../../../udjat-module-users) alert on sqlite database:

```xml
<?xml version="1.0" encoding="UTF-8" ?>
<config log-debug='yes' log-trace='yes'>

	<!-- The HTTP module implements the http client backend -->
	<module name='http' required='yes' />
	
	<!-- Implements user's monitor -->
	<module name='users' required='yes' />
	
	<!-- Load SQLite module -->
	<module name='sqlite' required='yes' dbname='alerts.db' />	

	<!-- Create 'sqlite+' url filter, setup database -->
	<sql name='sqlite' type='url-scheme' retry-interval='14400' update-timer='14400'>
	
		<init>
			create table if not exists alerts (id integer primary key, inserted timestamp default CURRENT_TIMESTAMP, url text, action text, payload text)
		</init>
		
		<insert>
			insert into alerts (url,action,payload) values (?,?,?)
		</insert>

		<select>
			select id,url,action,payload from alerts limit 1
		</select>
		
		<delete>
			delete from alerts where id=?
		</delete>

		<pending>
			select count (*) from alerts
		</pending>

	</sql>
	
	<!-- Declare an user monitor agent -->
	<users name='users' update-timer='60'>

		<!-- The URL is prefixed with 'sqlite+' to be stored on database -->
		<alert name='logout' event='logout' max-retries='1' action='post' url='sqlite+http://localhost'>
			{"user":"${username}","macaddress":"${macaddress}"}
		</alert>

	</users>
	
</config>
```

