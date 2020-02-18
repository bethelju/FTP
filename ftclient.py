#Name: Justin Bethel
#Course: CS 372
#Program Name: FTPCLIENT
#Program Description: The Python client file for an FTP connection
#Last Modified: 12:58pm 12/2/2019

from socket import *
import os
import sys
import time

#makeContact function sets up the initial "p" connection to the server
#It takes no arguments and returns the socket that holds the p connection
def makeContact():
	numCommandArgs = len(sys.argv) - 1
	if(numCommandArgs < 6 or numCommandArgs > 7):
		print("There needs to be between 6 and 7 arguments")
		return 0;
	serverName = sys.argv[1]
	serverPort = sys.argv[2]
	clientSocket = socket(AF_INET, SOCK_STREAM);
	try:
		host_ip = gethostbyname(serverName)
	except gaierror:
		print("There was an issue finding an ip for that serverName")
		return 0;
	clientSocket.connect((host_ip, int(serverPort)))
	return clientSocket;

#makeRequest function takes the clientSocket and sends the necessary command to the server
#It processes the user input and returns the data port
#Returns the data port
def makeRequest(clientSocket):
	command = sys.argv[3]
	argIndex = 4
	if((len(sys.argv)-1) == 7):
		command += " " + sys.argv[4]
		argIndex += 1
	dataPort = sys.argv[argIndex]
	command += " " + sys.argv[argIndex]

	#Take in login information and send to client
	argIndex += 1
	userName = sys.argv[argIndex]
	argIndex += 1
	password = sys.argv[argIndex]
	clientSocket.send(userName)
	time.sleep(1)
	clientSocket.send(password)
	if(clientSocket.recv(1024) == "No match"):
		print("Login info incorrect")
		exit(1)
	#If matching login information, continue
	clientSocket.send(command)
	return dataPort

#Function that handles the response from the server
#Takes the client socket and data port and functions as requested by command
def handleResponse(clientSocket, dataPort):
	#Take in next input from server
	socketMessage = clientSocket.recv(1024)
	#Prints error if server says there is no corresponding command
	if(socketMessage == "There is no command like that"):
		print(socketMessage)
	#If there is a corresponding command, open up another connection and handle input
	else:
		dataControlSocket = socket(AF_INET, SOCK_STREAM)
		dataControlSocket.bind(('', int(dataPort)))
		dataControlSocket.listen(1)
		dataControlConnectionSocket, addr = dataControlSocket.accept()
		#If the original command was -l
		if(sys.argv[3] == "-l"):
			#Take in and print the list of files until a special EOF message recieved
			commandResponseMsg = dataControlConnectionSocket.recv(1024)
			while(commandResponseMsg != "endOfInput" and commandResponseMsg != ""):
				print(commandResponseMsg)
				commandResponseMsg = dataControlConnectionSocket.recv(1024)	
		else:
			#If the command is to find a file
			fileFound = clientSocket.recv(1024)
			#If server responds that there is no file with that name, output so
			if(fileFound == "NO FILE WITH THAT NAME"):
				print(fileFound);
			#If there is a file with that name 
			else:
				#Check to see if we have a file with that name in our cwd and if so modify our 
				#output file name
				if(os.path.exists("./"+sys.argv[4])):
					outputFile = sys.argv[4].partition(".txt")[0]+"1"+".txt"
				else:
					outputFile = sys.argv[4]
				#open a file for output
				f = open(outputFile, 'w')
				#Take in message from server socket and write to file
				lineInMsg = dataControlConnectionSocket.recv(1024)
				while("endOfInput" not in lineInMsg):
					f.write(lineInMsg)
					#Split off any remaining bytes that are null
					#https://stackoverflow.com/questions/18970830/how-to-find-null-byte-in-a-string-in-python
					#Used above link to figure out how to specify null in python
					lineInMsg = dataControlConnectionSocket.recv(1024).split('\x00')[0]
				print("File Transfer Complete")
		dataControlConnectionSocket.close()

#Carry out all functions
def main():
	clientSocket = makeContact()
	dataPort = makeRequest(clientSocket)
	handleResponse(clientSocket, dataPort)
	clientSocket.close()
	
main()
