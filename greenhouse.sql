CREATE TABLE [data] (
    [time] DATETIME PRIMARY KEY,
    [temperature_1] FLOAT,
    [temperature_2] FLOAT,
    [humidity_1] FLOAT,
    [humidity_2] FLOAT,
    [fan_on] BOOLEAN,
    [heater_on] BOOLEAN
) WITHOUT ROWID;