DROP TABLE `rg_custom_tickets`;
CREATE TABLE `rg_custom_tickets` (
`ticketId`  mediumint NOT NULL ,
`questId`  mediumint NOT NULL ,
`priority`  varchar(25) NOT NULL ,
`status`  varchar(25) NOT NULL ,
PRIMARY KEY (`ticketId`)
);



